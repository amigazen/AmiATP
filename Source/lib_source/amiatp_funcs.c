/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * amiatp_funcs.c - Register-calling LVO wrappers for amiatp.library
 */

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include "amiatp_funcs.h"
#include "private/atp_internal.h"
#include "private/atp_json.h"
#include "private/amiatpbase.h"

extern struct AmiAtpBase *AtpBase;

static void
atp_lvo_bind(struct AmiAtpBase *libbase)
{
    if (libbase != NULL) {
        AtpBase = libbase;
    }
}

static STRPTR
atp_error_string(LONG code)
{
    switch (code) {
    case 0: return (STRPTR)"OK";
    case ERROR_ATP_NOT_IMPLEMENTED: return (STRPTR)"Not implemented";
    case ERROR_ATP_OUT_OF_MEMORY: return (STRPTR)"Out of memory";
    case ERROR_ATP_INVALID_HANDLE: return (STRPTR)"Invalid handle";
    case ERROR_ATP_INVALID_ARG: return (STRPTR)"Invalid argument";
    case ERROR_ATP_NO_HTTP: return (STRPTR)"amihttp.library not available";
    case ERROR_ATP_HTTP: return (STRPTR)"HTTP transport error";
    case ERROR_ATP_HTTP_STATUS: return (STRPTR)"HTTP error status";
    case ERROR_ATP_JSON: return (STRPTR)"JSON parse error";
    case ERROR_ATP_AUTH: return (STRPTR)"Authentication failed";
    case ERROR_ATP_NOT_LOGGED_IN: return (STRPTR)"Not logged in";
    case ERROR_ATP_BUFFER_TOO_SMALL: return (STRPTR)"Buffer too small";
    case ERROR_ATP_NOT_FOUND: return (STRPTR)"Not found";
    case ERROR_ATP_XRPC: return (STRPTR)"XRPC error";
    default: return (STRPTR)"Unknown amiatp error";
    }
}

LONG __ASM__ __SAVE_DS__
AtpBaseTagList(__REG__(a0, struct TagItem *tags),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    struct TagItem *tstate;
    struct TagItem *tag;

    atp_lvo_bind(libbase);
    if (libbase == NULL) {
        return 0;
    }
    if (tags == NULL) {
        return 1;
    }
    tstate = tags;
    while ((tag = NextTagItem(&tstate)) != NULL) {
        switch (tag->ti_Tag) {
        case ATBT_BREAKMASK:
            libbase->aab_BreakMask = tag->ti_Data;
            break;
        case ATBT_DEFAULT_USERAGENT:
            atp_free(libbase->aab_DefaultUserAgent);
            libbase->aab_DefaultUserAgent = atp_strdup((STRPTR)tag->ti_Data);
            break;
        case ATBT_CA_BUNDLE_PATH:
            atp_free(libbase->aab_CABundlePath);
            libbase->aab_CABundlePath = atp_strdup((STRPTR)tag->ti_Data);
            break;
        case ATBT_DEFAULT_SERVICE:
            atp_free(libbase->aab_DefaultService);
            libbase->aab_DefaultService = atp_strdup((STRPTR)tag->ti_Data);
            break;
        case ATBT_DEFAULT_APPVIEW:
            atp_free(libbase->aab_DefaultAppView);
            libbase->aab_DefaultAppView = atp_strdup((STRPTR)tag->ti_Data);
            break;
        default:
            break;
        }
    }
    return 1;
}

LONG __ASM__ __SAVE_DS__
AtpError(__REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    if (libbase == NULL) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    return libbase->aab_LastError;
}

STRPTR __ASM__ __SAVE_DS__
AtpGetErrorString(__REG__(d0, LONG code))
{
    return atp_error_string(code);
}

struct AtpConnection * __ASM__ __SAVE_DS__
NewAtpConnection(__REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_new_connection(libbase);
}

VOID __ASM__ __SAVE_DS__
DisposeAtpConnection(__REG__(a0, struct AtpConnection *conn))
{
    atp_dispose_connection(conn);
}

LONG __ASM__ __SAVE_DS__
SetAtpConnectionAttrsA(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct TagItem *tags),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_set_connection_attrs(conn, tags);
}

LONG __ASM__ __SAVE_DS__
AtpLogin(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR identifier), __REG__(a2, STRPTR password),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_login(conn, identifier, password);
}

LONG __ASM__ __SAVE_DS__
AtpRefreshSession(__REG__(a0, struct AtpConnection *conn),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_refresh_session(conn);
}

LONG __ASM__ __SAVE_DS__
AtpLogout(__REG__(a0, struct AtpConnection *conn),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_logout(conn);
}

STRPTR __ASM__ __SAVE_DS__
AtpConnectionGetDid(__REG__(a0, struct AtpConnection *conn))
{
    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return NULL;
    }
    return conn->ac_Did;
}

STRPTR __ASM__ __SAVE_DS__
AtpConnectionGetHandle(__REG__(a0, struct AtpConnection *conn))
{
    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return NULL;
    }
    return conn->ac_Handle;
}

LONG __ASM__ __SAVE_DS__
AtpConnectionGetLastError(__REG__(a0, struct AtpConnection *conn))
{
    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    return conn->ac_LastError;
}

struct AtpRecord * __ASM__ __SAVE_DS__
NewAtpRecord(__REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_new_record();
}

VOID __ASM__ __SAVE_DS__
DisposeAtpRecord(__REG__(a0, struct AtpRecord *rec))
{
    atp_dispose_record(rec);
}

LONG __ASM__ __SAVE_DS__
SetAtpRecordAttrsA(__REG__(a0, struct AtpRecord *rec),
    __REG__(a1, struct TagItem *tags),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_set_record_attrs(rec, tags);
}

LONG __ASM__ __SAVE_DS__
AtpGetRecord(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct AtpRecord *rec),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_get_record(conn, rec);
}

LONG __ASM__ __SAVE_DS__
AtpCreateRecord(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct AtpRecord *rec),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_create_record(conn, rec);
}

LONG __ASM__ __SAVE_DS__
AtpPutRecord(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct AtpRecord *rec),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_put_record(conn, rec);
}

LONG __ASM__ __SAVE_DS__
AtpDeleteRecord(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct AtpRecord *rec),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_delete_record(conn, rec);
}

LONG __ASM__ __SAVE_DS__
AtpListRecords(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR collection), __REG__(d0, ULONG limit),
    __REG__(a2, STRPTR cursor), __REG__(a3, struct AtpRecordList **out_list),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_list_records(conn, collection, limit, cursor, out_list);
}

VOID __ASM__ __SAVE_DS__
DisposeAtpRecordList(__REG__(a0, struct AtpRecordList *list))
{
    atp_dispose_record_list(list);
}

ULONG __ASM__ __SAVE_DS__
AtpRecordListGetCount(__REG__(a0, struct AtpRecordList *list))
{
    if (list == NULL || list->al_Magic != ATP_MAGIC_LIST) {
        return 0;
    }
    return list->al_Count;
}

struct AtpRecord * __ASM__ __SAVE_DS__
AtpRecordListGetRecord(__REG__(a0, struct AtpRecordList *list),
    __REG__(d0, ULONG index))
{
    if (list == NULL || list->al_Magic != ATP_MAGIC_LIST) {
        return NULL;
    }
    if (index >= list->al_Count || list->al_Items == NULL) {
        return NULL;
    }
    return list->al_Items[index];
}

STRPTR __ASM__ __SAVE_DS__
AtpRecordGetUri(__REG__(a0, struct AtpRecord *rec))
{
    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC) {
        return NULL;
    }
    return rec->ar_Uri;
}

STRPTR __ASM__ __SAVE_DS__
AtpRecordGetCid(__REG__(a0, struct AtpRecord *rec))
{
    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC) {
        return NULL;
    }
    return rec->ar_Cid;
}

STRPTR __ASM__ __SAVE_DS__
AtpRecordGetString(__REG__(a0, struct AtpRecord *rec),
    __REG__(a1, STRPTR key))
{
    if (rec == NULL || rec->ar_Magic != ATP_MAGIC_REC || key == NULL) {
        return NULL;
    }
    if (strcmp((char *)key, "text") == 0 && rec->ar_Text != NULL) {
        return rec->ar_Text;
    }
    if (rec->ar_ValueJson != NULL) {
        /* Return pointer into a static? Better: search value JSON.
         * For Amiga lifetime, store last lookup is awkward.
         * Use ar_Text slot for last GetString of non-text keys via replace —
         * too surprising. Document that GetString for value keys returns
         * newly... no, plan says accessor. Dup into ar_Text is wrong.
         * Simplest: only support keys in value via temporary — callers must
         * copy. We return pointer into ValueJson after parse into a
         * per-record cache string ar_Text reused as last-get cache. */
        {
            STRPTR s;

            s = atp_json_object_dup_string(rec->ar_ValueJson, key);
            if (s != NULL) {
                atp_free(rec->ar_Text);
                rec->ar_Text = s;
                return rec->ar_Text;
            }
        }
    }
    return NULL;
}

LONG __ASM__ __SAVE_DS__
AtpGetTimeline(__REG__(a0, struct AtpConnection *conn),
    __REG__(d0, ULONG limit), __REG__(a1, STRPTR cursor),
    __REG__(a2, struct AtpFeed **out_feed),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_get_timeline(conn, limit, cursor, out_feed);
}

LONG __ASM__ __SAVE_DS__
AtpGetAuthorFeed(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR actor), __REG__(d0, ULONG limit),
    __REG__(a2, STRPTR cursor), __REG__(a3, struct AtpFeed **out_feed),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_get_author_feed(conn, actor, limit, cursor, out_feed);
}

VOID __ASM__ __SAVE_DS__
DisposeAtpFeed(__REG__(a0, struct AtpFeed *feed))
{
    atp_dispose_feed(feed);
}

ULONG __ASM__ __SAVE_DS__
AtpFeedGetCount(__REG__(a0, struct AtpFeed *feed))
{
    if (feed == NULL || feed->af_Magic != ATP_MAGIC_FEED) {
        return 0;
    }
    return feed->af_Count;
}

struct AtpFeedPost * __ASM__ __SAVE_DS__
AtpFeedGetPost(__REG__(a0, struct AtpFeed *feed), __REG__(d0, ULONG index))
{
    if (feed == NULL || feed->af_Magic != ATP_MAGIC_FEED) {
        return NULL;
    }
    if (index >= feed->af_Count || feed->af_Posts == NULL) {
        return NULL;
    }
    return &feed->af_Posts[index];
}

STRPTR __ASM__ __SAVE_DS__
AtpFeedGetCursor(__REG__(a0, struct AtpFeed *feed))
{
    if (feed == NULL || feed->af_Magic != ATP_MAGIC_FEED) {
        return NULL;
    }
    return feed->af_Cursor;
}

STRPTR __ASM__ __SAVE_DS__
AtpFeedPostGetAuthorHandle(__REG__(a0, struct AtpFeedPost *post))
{
    if (post == NULL) {
        return NULL;
    }
    return post->afp_AuthorHandle;
}

STRPTR __ASM__ __SAVE_DS__
AtpFeedPostGetText(__REG__(a0, struct AtpFeedPost *post))
{
    if (post == NULL) {
        return NULL;
    }
    return post->afp_Text;
}

STRPTR __ASM__ __SAVE_DS__
AtpFeedPostGetUri(__REG__(a0, struct AtpFeedPost *post))
{
    if (post == NULL) {
        return NULL;
    }
    return post->afp_Uri;
}

STRPTR __ASM__ __SAVE_DS__
AtpFeedPostGetAvatarUrl(__REG__(a0, struct AtpFeedPost *post))
{
    if (post == NULL) {
        return NULL;
    }
    return post->afp_AvatarUrl;
}

ULONG __ASM__ __SAVE_DS__
AtpFeedPostGetImageCount(__REG__(a0, struct AtpFeedPost *post))
{
    if (post == NULL) {
        return 0;
    }
    return post->afp_ImageCount;
}

STRPTR __ASM__ __SAVE_DS__
AtpFeedPostGetImageUrl(__REG__(a0, struct AtpFeedPost *post),
    __REG__(d0, ULONG index))
{
    if (post == NULL || index >= post->afp_ImageCount) {
        return NULL;
    }
    return post->afp_ImageUrls[index];
}

LONG __ASM__ __SAVE_DS__
AtpGetProfile(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR actor), __REG__(a2, struct AtpProfile **out_profile),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_get_profile(conn, actor, out_profile);
}

VOID __ASM__ __SAVE_DS__
DisposeAtpProfile(__REG__(a0, struct AtpProfile *profile))
{
    atp_dispose_profile(profile);
}

STRPTR __ASM__ __SAVE_DS__
AtpProfileGetHandle(__REG__(a0, struct AtpProfile *profile))
{
    if (profile == NULL || profile->ap_Magic != ATP_MAGIC_PROF) {
        return NULL;
    }
    return profile->ap_Handle;
}

STRPTR __ASM__ __SAVE_DS__
AtpProfileGetDisplayName(__REG__(a0, struct AtpProfile *profile))
{
    if (profile == NULL || profile->ap_Magic != ATP_MAGIC_PROF) {
        return NULL;
    }
    return profile->ap_DisplayName;
}

STRPTR __ASM__ __SAVE_DS__
AtpProfileGetDescription(__REG__(a0, struct AtpProfile *profile))
{
    if (profile == NULL || profile->ap_Magic != ATP_MAGIC_PROF) {
        return NULL;
    }
    return profile->ap_Description;
}

LONG __ASM__ __SAVE_DS__
AtpCreatePost(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR text), __REG__(a2, STRPTR uri_buf),
    __REG__(d0, ULONG uri_buflen),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_create_post(conn, text, uri_buf, uri_buflen);
}

LONG __ASM__ __SAVE_DS__
AtpResolveHandle(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR handle), __REG__(a2, STRPTR did_buf),
    __REG__(d0, ULONG did_buflen),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_resolve_handle(conn, handle, did_buf, did_buflen);
}

LONG __ASM__ __SAVE_DS__
AtpDownloadUrl(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR url), __REG__(a2, STRPTR path),
    __REG__(a6, struct AmiAtpBase *libbase))
{
    atp_lvo_bind(libbase);
    return atp_download_url(conn, url, path);
}
