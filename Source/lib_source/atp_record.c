/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * atp_record.c - Generic com.atproto.repo.* CRUD
 */

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>

#include "private/atp_internal.h"
#include "private/atp_json.h"

static LONG
atp_replace_str(STRPTR *slot, STRPTR value)
{
    STRPTR n;

    if (slot == NULL) {
        return ERROR_ATP_INVALID_ARG;
    }
    if (value == NULL) {
        atp_free(*slot);
        *slot = NULL;
        return 0;
    }
    n = atp_strdup(value);
    if (n == NULL) {
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    atp_free(*slot);
    *slot = n;
    return 0;
}

struct AtpRecord *
atp_new_record(void)
{
    struct AtpRecord *rec;

    rec = (struct AtpRecord *)atp_alloc(sizeof(*rec));
    if (rec == NULL) {
        return NULL;
    }
    rec->ar_Magic = ATP_MAGIC_REC;
    return rec;
}

void
atp_dispose_record(struct AtpRecord *rec)
{
    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC) {
        return;
    }
    rec->ar_Magic = 0;
    atp_free(rec->ar_Collection);
    atp_free(rec->ar_Rkey);
    atp_free(rec->ar_Repo);
    atp_free(rec->ar_JsonBody);
    atp_free(rec->ar_Text);
    atp_free(rec->ar_Uri);
    atp_free(rec->ar_Cid);
    atp_free(rec->ar_ValueJson);
    atp_free(rec);
}

LONG
atp_set_record_attrs(struct AtpRecord *rec, struct TagItem *tags)
{
    struct TagItem *tstate;
    struct TagItem *tag;
    LONG err;

    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (tags == NULL) {
        return 0;
    }
    tstate = tags;
    while ((tag = NextTagItem(&tstate)) != NULL) {
        switch (tag->ti_Tag) {
        case ATPR_COLLECTION:
            err = atp_replace_str(&rec->ar_Collection, (STRPTR)tag->ti_Data);
            break;
        case ATPR_RKEY:
            err = atp_replace_str(&rec->ar_Rkey, (STRPTR)tag->ti_Data);
            break;
        case ATPR_REPO:
            err = atp_replace_str(&rec->ar_Repo, (STRPTR)tag->ti_Data);
            break;
        case ATPR_JSON_BODY:
            err = atp_replace_str(&rec->ar_JsonBody, (STRPTR)tag->ti_Data);
            break;
        case ATPR_TEXT:
            err = atp_replace_str(&rec->ar_Text, (STRPTR)tag->ti_Data);
            break;
        default:
            err = 0;
            break;
        }
        if (err != 0) {
            return err;
        }
    }
    return 0;
}

static STRPTR
atp_rec_repo(struct AtpConnection *conn, struct AtpRecord *rec)
{
    if (rec->ar_Repo != NULL && rec->ar_Repo[0] != '\0') {
        return rec->ar_Repo;
    }
    return conn->ac_Did;
}

LONG
atp_get_record(struct AtpConnection *conn, struct AtpRecord *rec)
{
    UBYTE query[ATP_MAX_URL];
    STRPTR body;
    LONG http_status;
    LONG err;
    STRPTR repo;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (rec->ar_Collection == NULL || rec->ar_Rkey == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    repo = atp_rec_repo(conn, rec);
    if (repo == NULL) {
        atp_conn_error(conn, ERROR_ATP_NOT_LOGGED_IN);
        return ERROR_ATP_NOT_LOGGED_IN;
    }
    if (!atp_build_query3((STRPTR)query, sizeof(query),
            "repo", repo,
            "collection", rec->ar_Collection,
            "rkey", rec->ar_Rkey)) {
        atp_conn_error(conn, ERROR_ATP_BUFFER_TOO_SMALL);
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    body = NULL;
    err = atp_xrpc_authed(conn, conn->ac_Service, "GET",
        "com.atproto.repo.getRecord", (STRPTR)query, NULL,
        &body, &http_status);
    if (err != 0) {
        atp_free(body);
        atp_conn_error(conn, err);
        return err;
    }
    atp_free(rec->ar_Uri);
    atp_free(rec->ar_Cid);
    atp_free(rec->ar_ValueJson);
    rec->ar_Uri = atp_json_object_dup_string(body, "uri");
    rec->ar_Cid = atp_json_object_dup_string(body, "cid");
    {
        STRPTR v;

        v = atp_json_find_key(body, "value");
        if (v != NULL) {
            rec->ar_ValueJson = atp_json_dup_value(v);
        }
    }
    atp_free(body);
    atp_conn_error(conn, 0);
    return 0;
}

LONG
atp_create_record(struct AtpConnection *conn, struct AtpRecord *rec)
{
    STRPTR record_json;
    STRPTR body_in;
    STRPTR body_out;
    LONG http_status;
    LONG err;
    STRPTR repo;
    UBYTE created[32];
    BOOL free_record_json;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (rec->ar_Collection == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    repo = atp_rec_repo(conn, rec);
    if (repo == NULL) {
        atp_conn_error(conn, ERROR_ATP_NOT_LOGGED_IN);
        return ERROR_ATP_NOT_LOGGED_IN;
    }

    free_record_json = FALSE;
    record_json = rec->ar_JsonBody;
    if (record_json == NULL && rec->ar_Text != NULL) {
        atp_now_iso8601(conn, (STRPTR)created, sizeof(created));
        record_json = atp_json_build_post_record(rec->ar_Text, (STRPTR)created);
        free_record_json = TRUE;
        if (record_json == NULL) {
            atp_conn_error(conn, ERROR_ATP_OUT_OF_MEMORY);
            return ERROR_ATP_OUT_OF_MEMORY;
        }
    }
    if (record_json == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }

    body_in = atp_json_build_create_record(repo, rec->ar_Collection,
        rec->ar_Rkey, record_json);
    if (free_record_json) {
        atp_free(record_json);
    }
    if (body_in == NULL) {
        atp_conn_error(conn, ERROR_ATP_OUT_OF_MEMORY);
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    body_out = NULL;
    err = atp_xrpc_authed(conn, conn->ac_Service, "POST",
        "com.atproto.repo.createRecord", NULL, body_in,
        &body_out, &http_status);
    atp_free(body_in);
    if (err != 0) {
        atp_free(body_out);
        atp_conn_error(conn, err);
        return err;
    }
    atp_free(rec->ar_Uri);
    atp_free(rec->ar_Cid);
    rec->ar_Uri = atp_json_object_dup_string(body_out, "uri");
    rec->ar_Cid = atp_json_object_dup_string(body_out, "cid");
    atp_free(body_out);
    if (rec->ar_Uri == NULL || rec->ar_Uri[0] == '\0') {
        atp_verbose(conn, "createRecord OK status but missing uri\n");
        atp_conn_error(conn, ERROR_ATP_XRPC);
        return ERROR_ATP_XRPC;
    }
    atp_verbose(conn, "createRecord uri=%s\n", rec->ar_Uri);
    atp_conn_error(conn, 0);
    return 0;
}

LONG
atp_put_record(struct AtpConnection *conn, struct AtpRecord *rec)
{
    STRPTR record_json;
    STRPTR body_in;
    STRPTR body_out;
    LONG http_status;
    LONG err;
    STRPTR repo;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (rec->ar_Collection == NULL || rec->ar_Rkey == NULL ||
        rec->ar_JsonBody == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    repo = atp_rec_repo(conn, rec);
    if (repo == NULL) {
        atp_conn_error(conn, ERROR_ATP_NOT_LOGGED_IN);
        return ERROR_ATP_NOT_LOGGED_IN;
    }
    record_json = rec->ar_JsonBody;
    body_in = atp_json_build_put_record(repo, rec->ar_Collection,
        rec->ar_Rkey, record_json);
    if (body_in == NULL) {
        atp_conn_error(conn, ERROR_ATP_OUT_OF_MEMORY);
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    body_out = NULL;
    err = atp_xrpc_authed(conn, conn->ac_Service, "POST",
        "com.atproto.repo.putRecord", NULL, body_in,
        &body_out, &http_status);
    atp_free(body_in);
    if (err != 0) {
        atp_free(body_out);
        atp_conn_error(conn, err);
        return err;
    }
    atp_free(rec->ar_Uri);
    atp_free(rec->ar_Cid);
    rec->ar_Uri = atp_json_object_dup_string(body_out, "uri");
    rec->ar_Cid = atp_json_object_dup_string(body_out, "cid");
    atp_free(body_out);
    atp_conn_error(conn, 0);
    return 0;
}

LONG
atp_delete_record(struct AtpConnection *conn, struct AtpRecord *rec)
{
    UBYTE body_in[512];
    ULONG used;
    STRPTR body_out;
    LONG http_status;
    LONG err;
    STRPTR repo;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (rec->ar_Collection == NULL || rec->ar_Rkey == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    repo = atp_rec_repo(conn, rec);
    if (repo == NULL) {
        atp_conn_error(conn, ERROR_ATP_NOT_LOGGED_IN);
        return ERROR_ATP_NOT_LOGGED_IN;
    }
    strcpy((char *)body_in, "{\"repo\":");
    used = (ULONG)strlen((char *)body_in);
    if (!atp_json_append_escaped((STRPTR)body_in, sizeof(body_in), &used, repo)) {
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    strcpy((char *)body_in + used, ",\"collection\":");
    used = (ULONG)strlen((char *)body_in);
    if (!atp_json_append_escaped((STRPTR)body_in, sizeof(body_in), &used,
            rec->ar_Collection)) {
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    strcpy((char *)body_in + used, ",\"rkey\":");
    used = (ULONG)strlen((char *)body_in);
    if (!atp_json_append_escaped((STRPTR)body_in, sizeof(body_in), &used,
            rec->ar_Rkey)) {
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    if (used + 2 >= sizeof(body_in)) {
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    body_in[used++] = '}';
    body_in[used] = '\0';

    body_out = NULL;
    err = atp_xrpc_authed(conn, conn->ac_Service, "POST",
        "com.atproto.repo.deleteRecord", NULL, (STRPTR)body_in,
        &body_out, &http_status);
    atp_free(body_out);
    if (err != 0) {
        atp_conn_error(conn, err);
        return err;
    }
    atp_conn_error(conn, 0);
    return 0;
}

void
atp_dispose_record_list(struct AtpRecordList *list)
{
    ULONG i;

    if (list == NULL || list->al_Magic != ATP_MAGIC_LIST) {
        return;
    }
    if (list->al_Items != NULL) {
        for (i = 0; i < list->al_Count; i++) {
            atp_dispose_record(list->al_Items[i]);
        }
        atp_free(list->al_Items);
    }
    atp_free(list->al_Cursor);
    list->al_Magic = 0;
    atp_free(list);
}

LONG
atp_list_records(struct AtpConnection *conn, STRPTR collection, ULONG limit,
    STRPTR cursor, struct AtpRecordList **out_list)
{
    UBYTE query[ATP_MAX_URL];
    UBYTE limstr[16];
    STRPTR body;
    LONG http_status;
    LONG err;
    ULONG count;
    ULONG i;
    ULONG take;
    struct AtpRecordList *list;
    STRPTR repo;

    if (out_list != NULL) {
        *out_list = NULL;
    }
    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN || out_list == NULL) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (collection == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    repo = conn->ac_Did;
    if (repo == NULL) {
        atp_conn_error(conn, ERROR_ATP_NOT_LOGGED_IN);
        return ERROR_ATP_NOT_LOGGED_IN;
    }
    if (limit == 0 || limit > ATP_MAX_LIST_RECS) {
        limit = ATP_MAX_LIST_RECS;
    }
    sprintf((char *)limstr, "%lu", (unsigned long)limit);
    if (cursor != NULL && cursor[0] != '\0') {
        if (!atp_build_query3((STRPTR)query, sizeof(query),
                "repo", repo, "collection", collection, "limit", limstr)) {
            return ERROR_ATP_BUFFER_TOO_SMALL;
        }
        /* append cursor manually */
        {
            ULONG u;

            u = (ULONG)strlen((char *)query);
            if (u + 16 + strlen(cursor) >= sizeof(query)) {
                return ERROR_ATP_BUFFER_TOO_SMALL;
            }
            strcat((char *)query, "&cursor=");
            strcat((char *)query, (char *)cursor);
        }
    } else {
        if (!atp_build_query3((STRPTR)query, sizeof(query),
                "repo", repo, "collection", collection, "limit", limstr)) {
            return ERROR_ATP_BUFFER_TOO_SMALL;
        }
    }

    body = NULL;
    err = atp_xrpc_authed(conn, conn->ac_Service, "GET",
        "com.atproto.repo.listRecords", (STRPTR)query, NULL,
        &body, &http_status);
    if (err != 0) {
        atp_free(body);
        atp_conn_error(conn, err);
        return err;
    }

    count = atp_json_array_count(body, "records");
    take = count;
    if (take > limit) {
        take = limit;
    }
    list = (struct AtpRecordList *)atp_alloc(sizeof(*list));
    if (list == NULL) {
        atp_free(body);
        atp_conn_error(conn, ERROR_ATP_OUT_OF_MEMORY);
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    list->al_Magic = ATP_MAGIC_LIST;
    list->al_Cursor = atp_json_object_dup_string(body, "cursor");
    if (take > 0) {
        list->al_Items = (struct AtpRecord **)atp_alloc(
            sizeof(struct AtpRecord *) * take);
        if (list->al_Items == NULL) {
            atp_dispose_record_list(list);
            atp_free(body);
            atp_conn_error(conn, ERROR_ATP_OUT_OF_MEMORY);
            return ERROR_ATP_OUT_OF_MEMORY;
        }
    }
    for (i = 0; i < take; i++) {
        STRPTR es;
        STRPTR ee;
        struct AtpRecord *r;

        if (!atp_json_array_nth(body, "records", i, &es, &ee)) {
            break;
        }
        r = atp_new_record();
        if (r == NULL) {
            break;
        }
        r->ar_Uri = atp_json_fragment_dup_string(es, ee, "uri");
        r->ar_Cid = atp_json_fragment_dup_string(es, ee, "cid");
        r->ar_Collection = atp_strdup(collection);
        list->al_Items[list->al_Count++] = r;
    }
    atp_free(body);
    *out_list = list;
    atp_conn_error(conn, 0);
    return 0;
}
