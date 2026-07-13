/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * atp_bsky.c - Bluesky app.bsky.* schema helpers
 */

#include <exec/types.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>

#include "private/atp_internal.h"
#include "private/atp_json.h"

static void
atp_feed_post_clear(struct AtpFeedPost *post)
{
    ULONG i;

    atp_free(post->afp_AuthorHandle);
    atp_free(post->afp_AuthorName);
    atp_free(post->afp_Text);
    atp_free(post->afp_Uri);
    atp_free(post->afp_CreatedAt);
    atp_free(post->afp_AvatarUrl);
    for (i = 0; i < ATP_MAX_FEED_IMAGES; i++) {
        atp_free(post->afp_ImageUrls[i]);
        post->afp_ImageUrls[i] = NULL;
    }
    post->afp_AuthorHandle = NULL;
    post->afp_AuthorName = NULL;
    post->afp_Text = NULL;
    post->afp_Uri = NULL;
    post->afp_CreatedAt = NULL;
    post->afp_AvatarUrl = NULL;
    post->afp_ImageCount = 0;
}

/*
 * Collect thumb URLs from an embed images[] array (view form).
 */
static void
atp_collect_image_thumbs(STRPTR obj, struct AtpFeedPost *post)
{
    ULONG n;
    ULONG i;

    if (obj == NULL || post == NULL) {
        return;
    }
    n = atp_json_array_count(obj, "images");
    for (i = 0; i < n && post->afp_ImageCount < ATP_MAX_FEED_IMAGES; i++) {
        STRPTR es;
        STRPTR ee;
        STRPTR thumb;

        if (!atp_json_array_nth(obj, "images", i, &es, &ee)) {
            break;
        }
        thumb = atp_json_fragment_dup_string(es, ee, "thumb");
        if (thumb == NULL) {
            thumb = atp_json_fragment_dup_string(es, ee, "fullsize");
        }
        if (thumb != NULL) {
            post->afp_ImageUrls[post->afp_ImageCount++] = thumb;
        }
    }
}

static void
atp_parse_post_media(STRPTR post_obj, struct AtpFeedPost *post)
{
    STRPTR emb;
    STRPTR media;

    if (post_obj == NULL || post == NULL) {
        return;
    }
    emb = atp_json_find_key(post_obj, "embed");
    if (emb == NULL) {
        return;
    }
    emb = atp_json_skip(emb);
    if (*emb != '{') {
        return;
    }
    /* app.bsky.embed.images#view */
    atp_collect_image_thumbs(emb, post);
    if (post->afp_ImageCount > 0) {
        return;
    }
    /* app.bsky.embed.recordWithMedia#view → media.images */
    media = atp_json_find_key(emb, "media");
    if (media != NULL) {
        media = atp_json_skip(media);
        if (*media == '{') {
            atp_collect_image_thumbs(media, post);
        }
    }
    if (post->afp_ImageCount > 0) {
        return;
    }
    /* app.bsky.embed.external#view → external.thumb */
    {
        STRPTR ext;
        STRPTR thumb;

        ext = atp_json_find_key(emb, "external");
        if (ext != NULL) {
            ext = atp_json_skip(ext);
            if (*ext == '{') {
                thumb = atp_json_object_dup_string(ext, "thumb");
                if (thumb != NULL) {
                    post->afp_ImageUrls[post->afp_ImageCount++] = thumb;
                    return;
                }
            }
        }
        thumb = atp_json_object_dup_string(emb, "thumb");
        if (thumb != NULL) {
            post->afp_ImageUrls[post->afp_ImageCount++] = thumb;
        }
    }
}

void
atp_dispose_feed(struct AtpFeed *feed)
{
    ULONG i;

    if (feed == NULL || feed->af_Magic != ATP_MAGIC_FEED) {
        return;
    }
    if (feed->af_Posts != NULL) {
        for (i = 0; i < feed->af_Count; i++) {
            atp_feed_post_clear(&feed->af_Posts[i]);
        }
        atp_free(feed->af_Posts);
    }
    atp_free(feed->af_Cursor);
    feed->af_Magic = 0;
    atp_free(feed);
}

static LONG
atp_parse_feed(STRPTR body, struct AtpFeed **out_feed)
{
    ULONG count;
    ULONG take;
    ULONG i;
    struct AtpFeed *feed;

    if (out_feed == NULL) {
        return ERROR_ATP_INVALID_ARG;
    }
    *out_feed = NULL;
    count = atp_json_array_count(body, "feed");
    take = count;
    if (take > ATP_MAX_FEED_POSTS) {
        take = ATP_MAX_FEED_POSTS;
    }
    feed = (struct AtpFeed *)atp_alloc(sizeof(*feed));
    if (feed == NULL) {
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    feed->af_Magic = ATP_MAGIC_FEED;
    feed->af_Cursor = atp_json_object_dup_string(body, "cursor");
    if (take > 0) {
        feed->af_Posts = (struct AtpFeedPost *)atp_alloc(
            sizeof(struct AtpFeedPost) * take);
        if (feed->af_Posts == NULL) {
            atp_dispose_feed(feed);
            return ERROR_ATP_OUT_OF_MEMORY;
        }
    }
    for (i = 0; i < take; i++) {
        STRPTR es;
        STRPTR ee;
        struct AtpFeedPost *post;

        if (!atp_json_array_nth(body, "feed", i, &es, &ee)) {
            break;
        }
        post = &feed->af_Posts[feed->af_Count];
        post->afp_Uri = atp_json_fragment_dup_string2(es, ee, "post", "uri");
        post->afp_AuthorHandle = atp_json_fragment_dup_string2(es, ee,
            "post", "author");
        /* author is object — re-parse handle from nested author */
        {
            STRPTR author_obj;
            ULONG len;
            STRPTR tmp;

            len = (ULONG)(ee - es);
            tmp = (STRPTR)atp_alloc(len + 1);
            if (tmp != NULL) {
                CopyMem((APTR)es, tmp, len);
                tmp[len] = '\0';
                author_obj = NULL;
                {
                    STRPTR pv;
                    STRPTR av;

                    pv = atp_json_find_key(tmp, "post");
                    if (pv != NULL) {
                        pv = atp_json_skip(pv);
                        if (*pv == '{') {
                            av = atp_json_find_key(pv, "author");
                            if (av != NULL) {
                                atp_free(post->afp_AuthorHandle);
                                post->afp_AuthorHandle =
                                    atp_json_object_dup_string(
                                        atp_json_skip(av), "handle");
                                /* find_key on author object needs object start */
                                {
                                    STRPTR ao;

                                    ao = atp_json_skip(av);
                                    if (*ao == '{') {
                                        atp_free(post->afp_AuthorHandle);
                                        post->afp_AuthorHandle =
                                            atp_json_object_dup_string(ao, "handle");
                                        post->afp_AuthorName =
                                            atp_json_object_dup_string(ao,
                                                "displayName");
                                        post->afp_AvatarUrl =
                                            atp_json_object_dup_string(ao,
                                                "avatar");
                                    }
                                }
                            }
                            {
                                STRPTR rec;

                                rec = atp_json_find_key(pv, "record");
                                if (rec != NULL) {
                                    rec = atp_json_skip(rec);
                                    if (*rec == '{') {
                                        post->afp_Text =
                                            atp_json_object_dup_string(rec, "text");
                                        post->afp_CreatedAt =
                                            atp_json_object_dup_string(rec,
                                                "createdAt");
                                    }
                                }
                            }
                            atp_parse_post_media(pv, post);
                            {
                                STRPTR uri;

                                uri = atp_json_find_key(pv, "uri");
                                if (uri != NULL) {
                                    atp_free(post->afp_Uri);
                                    {
                                        UBYTE ubuf[ATP_MAX_STR];

                                        if (atp_json_get_string(uri, (STRPTR)ubuf,
                                                sizeof(ubuf))) {
                                            post->afp_Uri = atp_strdup(
                                                (STRPTR)ubuf);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                atp_free(tmp);
            }
        }
        feed->af_Count++;
    }
    *out_feed = feed;
    return 0;
}

LONG
atp_get_timeline(struct AtpConnection *conn, ULONG limit, STRPTR cursor,
    struct AtpFeed **out_feed)
{
    UBYTE query[ATP_MAX_URL];
    UBYTE limstr[16];
    STRPTR body;
    LONG http_status;
    LONG err;

    if (out_feed != NULL) {
        *out_feed = NULL;
    }
    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN || out_feed == NULL) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (limit == 0 || limit > ATP_MAX_FEED_POSTS) {
        limit = 20;
    }
    sprintf((char *)limstr, "%lu", (unsigned long)limit);
    if (cursor != NULL && cursor[0] != '\0') {
        if (!atp_build_query2((STRPTR)query, sizeof(query),
                "limit", limstr, "cursor", cursor)) {
            return ERROR_ATP_BUFFER_TOO_SMALL;
        }
    } else {
        if (!atp_build_query2((STRPTR)query, sizeof(query),
                "limit", limstr, NULL, NULL)) {
            return ERROR_ATP_BUFFER_TOO_SMALL;
        }
    }
    body = NULL;
    /*
     * Authenticated app.bsky.* calls go to the PDS (proxies to AppView).
     * Hitting public.api.bsky.app directly often returns 401 for session JWTs.
     */
    err = atp_xrpc_authed(conn, conn->ac_Service, "GET",
        "app.bsky.feed.getTimeline", (STRPTR)query, NULL,
        &body, &http_status);
    if (err != 0) {
        atp_free(body);
        atp_conn_error(conn, err);
        return err;
    }
    err = atp_parse_feed(body, out_feed);
    atp_free(body);
    if (err != 0) {
        atp_conn_error(conn, err);
        return err;
    }
    atp_conn_error(conn, 0);
    return 0;
}

LONG
atp_get_author_feed(struct AtpConnection *conn, STRPTR actor, ULONG limit,
    STRPTR cursor, struct AtpFeed **out_feed)
{
    UBYTE query[ATP_MAX_URL];
    UBYTE limstr[16];
    STRPTR body;
    LONG http_status;
    LONG err;

    if (out_feed != NULL) {
        *out_feed = NULL;
    }
    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN || out_feed == NULL) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (actor == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    if (limit == 0 || limit > ATP_MAX_FEED_POSTS) {
        limit = 20;
    }
    sprintf((char *)limstr, "%lu", (unsigned long)limit);
    if (cursor != NULL && cursor[0] != '\0') {
        if (!atp_build_query3((STRPTR)query, sizeof(query),
                "actor", actor, "limit", limstr, "cursor", cursor)) {
            return ERROR_ATP_BUFFER_TOO_SMALL;
        }
    } else {
        if (!atp_build_query2((STRPTR)query, sizeof(query),
                "actor", actor, "limit", limstr)) {
            return ERROR_ATP_BUFFER_TOO_SMALL;
        }
    }
    body = NULL;
    if (conn->ac_LoggedIn) {
        err = atp_xrpc_authed(conn, conn->ac_Service, "GET",
            "app.bsky.feed.getAuthorFeed", (STRPTR)query, NULL,
            &body, &http_status);
    } else {
        err = atp_xrpc_request(conn, conn->ac_AppView, "GET",
            "app.bsky.feed.getAuthorFeed", (STRPTR)query, NULL,
            FALSE, FALSE, &body, &http_status);
    }
    if (err != 0) {
        atp_free(body);
        atp_conn_error(conn, err);
        return err;
    }
    err = atp_parse_feed(body, out_feed);
    atp_free(body);
    if (err != 0) {
        atp_conn_error(conn, err);
        return err;
    }
    atp_conn_error(conn, 0);
    return 0;
}

void
atp_dispose_profile(struct AtpProfile *profile)
{
    if (profile == NULL || profile->ap_Magic != ATP_MAGIC_PROF) {
        return;
    }
    atp_free(profile->ap_Did);
    atp_free(profile->ap_Handle);
    atp_free(profile->ap_DisplayName);
    atp_free(profile->ap_Description);
    profile->ap_Magic = 0;
    atp_free(profile);
}

LONG
atp_get_profile(struct AtpConnection *conn, STRPTR actor,
    struct AtpProfile **out_profile)
{
    UBYTE query[ATP_MAX_URL];
    STRPTR body;
    LONG http_status;
    LONG err;
    struct AtpProfile *prof;

    if (out_profile != NULL) {
        *out_profile = NULL;
    }
    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN || out_profile == NULL) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (actor == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    if (!atp_build_query2((STRPTR)query, sizeof(query),
            "actor", actor, NULL, NULL)) {
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    body = NULL;
    err = atp_xrpc_request(conn, conn->ac_AppView, "GET",
        "app.bsky.actor.getProfile", (STRPTR)query, NULL,
        conn->ac_LoggedIn, FALSE, &body, &http_status);
    if (err != 0) {
        atp_free(body);
        atp_conn_error(conn, err);
        return err;
    }
    prof = (struct AtpProfile *)atp_alloc(sizeof(*prof));
    if (prof == NULL) {
        atp_free(body);
        atp_conn_error(conn, ERROR_ATP_OUT_OF_MEMORY);
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    prof->ap_Magic = ATP_MAGIC_PROF;
    prof->ap_Did = atp_json_object_dup_string(body, "did");
    prof->ap_Handle = atp_json_object_dup_string(body, "handle");
    prof->ap_DisplayName = atp_json_object_dup_string(body, "displayName");
    prof->ap_Description = atp_json_object_dup_string(body, "description");
    atp_free(body);
    *out_profile = prof;
    atp_conn_error(conn, 0);
    return 0;
}

LONG
atp_create_post(struct AtpConnection *conn, STRPTR text,
    STRPTR uri_buf, ULONG uri_buflen)
{
    struct AtpRecord *rec;
    LONG err;
    struct TagItem tags[4];

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (text == NULL || text[0] == '\0') {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    if (strlen(text) > ATP_MAX_POST_TEXT) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    rec = atp_new_record();
    if (rec == NULL) {
        atp_conn_error(conn, ERROR_ATP_OUT_OF_MEMORY);
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    tags[0].ti_Tag = ATPR_COLLECTION;
    tags[0].ti_Data = (ULONG)"app.bsky.feed.post";
    tags[1].ti_Tag = ATPR_TEXT;
    tags[1].ti_Data = (ULONG)text;
    tags[2].ti_Tag = TAG_DONE;
    tags[2].ti_Data = 0;
    err = atp_set_record_attrs(rec, tags);
    if (err != 0) {
        atp_dispose_record(rec);
        atp_conn_error(conn, err);
        return err;
    }
    err = atp_create_record(conn, rec);
    if (err == 0 && uri_buf != NULL && uri_buflen > 0) {
        if (rec->ar_Uri != NULL) {
            strncpy((char *)uri_buf, (char *)rec->ar_Uri,
                (size_t)(uri_buflen - 1));
            uri_buf[uri_buflen - 1] = '\0';
        } else {
            uri_buf[0] = '\0';
        }
    }
    atp_dispose_record(rec);
    return err;
}

LONG
atp_resolve_handle(struct AtpConnection *conn, STRPTR handle,
    STRPTR did_buf, ULONG did_buflen)
{
    UBYTE query[ATP_MAX_URL];
    STRPTR body;
    LONG http_status;
    LONG err;
    UBYTE didtmp[ATP_MAX_DID];

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (handle == NULL || did_buf == NULL || did_buflen < 8) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    if (!atp_build_query2((STRPTR)query, sizeof(query),
            "handle", handle, NULL, NULL)) {
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    body = NULL;
    err = atp_xrpc_request(conn, conn->ac_Service, "GET",
        "com.atproto.identity.resolveHandle", (STRPTR)query, NULL,
        FALSE, FALSE, &body, &http_status);
    if (err != 0) {
        atp_free(body);
        atp_conn_error(conn, err);
        return err;
    }
    if (!atp_json_object_get_string(body, "did", (STRPTR)didtmp, sizeof(didtmp))) {
        atp_free(body);
        atp_conn_error(conn, ERROR_ATP_JSON);
        return ERROR_ATP_JSON;
    }
    atp_free(body);
    if (strlen((char *)didtmp) + 1 > did_buflen) {
        atp_conn_error(conn, ERROR_ATP_BUFFER_TOO_SMALL);
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    strcpy((char *)did_buf, (char *)didtmp);
    atp_conn_error(conn, 0);
    return 0;
}
