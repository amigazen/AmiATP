/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * atp_internal.h - Shared internal types for amiatp.library
 */

#ifndef ATP_PRIVATE_ATP_INTERNAL_H
#define ATP_PRIVATE_ATP_INTERNAL_H

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/amiatp.h>
#include <libraries/amihttp.h>
#include "amiatpbase.h"

#define ATP_MAGIC_CONN   0x41545043UL  /* 'ATPC' */
#define ATP_MAGIC_REC    0x41545052UL  /* 'ATPR' */
#define ATP_MAGIC_LIST   0x4154504CUL  /* 'ATPL' */
#define ATP_MAGIC_FEED   0x41545046UL  /* 'ATPF' */
#define ATP_MAGIC_PROF   0x41545050UL  /* 'ATPP' */

#define ATP_DEFAULT_SERVICE  "https://bsky.social"
#define ATP_DEFAULT_APPVIEW  "https://public.api.bsky.app"
#define ATP_DEFAULT_UA       "amiatp/1.0 (Amiga)"
#define ATP_MAX_BODY         (512UL * 1024UL)
#define ATP_MAX_URL          1024
#define ATP_MAX_STR          512
#define ATP_MAX_DID          128
#define ATP_MAX_HANDLE       256
#define ATP_MAX_JWT          4096
#define ATP_MAX_FEED_POSTS   50
#define ATP_MAX_FEED_IMAGES  4
#define ATP_MAX_LIST_RECS    50
#define ATP_MAX_POST_TEXT    300
#define ATP_MAX_DOWNLOAD     (512UL * 1024UL)

struct AtpConnection
{
    ULONG                   ac_Magic;
    struct AmiAtpBase      *ac_Base;
    struct HttpSession     *ac_HttpSession;
    STRPTR                  ac_Service;
    STRPTR                  ac_AppView;
    STRPTR                  ac_CABundle;
    STRPTR                  ac_UserAgent;
    ULONG                   ac_ConnectTimeout;
    ULONG                   ac_ReadTimeout;
    STRPTR                  ac_AccessJwt;
    STRPTR                  ac_RefreshJwt;
    STRPTR                  ac_Did;
    STRPTR                  ac_Handle;
    LONG                    ac_LastError;
    BOOL                    ac_LoggedIn;
    BOOL                    ac_Verbose;
    /* Last HTTP Date header as ISO-8601 UTC (Amiga clocks are often 1978). */
    UBYTE                   ac_NetworkIso8601[32];
};

struct AtpRecord
{
    ULONG                   ar_Magic;
    STRPTR                  ar_Collection;
    STRPTR                  ar_Rkey;
    STRPTR                  ar_Repo;
    STRPTR                  ar_JsonBody;
    STRPTR                  ar_Text;
    STRPTR                  ar_Uri;
    STRPTR                  ar_Cid;
    STRPTR                  ar_ValueJson;
};

struct AtpRecordList
{
    ULONG                   al_Magic;
    ULONG                   al_Count;
    struct AtpRecord      **al_Items;
    STRPTR                  al_Cursor;
};

struct AtpFeedPost
{
    STRPTR                  afp_AuthorHandle;
    STRPTR                  afp_AuthorName;
    STRPTR                  afp_Text;
    STRPTR                  afp_Uri;
    STRPTR                  afp_CreatedAt;
    STRPTR                  afp_AvatarUrl;
    ULONG                   afp_ImageCount;
    STRPTR                  afp_ImageUrls[ATP_MAX_FEED_IMAGES];
    /* app.bsky.embed.external#view — OpenGraph-style link card */
    STRPTR                  afp_ExtUri;
    STRPTR                  afp_ExtTitle;
    STRPTR                  afp_ExtDescription;
    STRPTR                  afp_ExtThumb;
};

struct AtpFeed
{
    ULONG                   af_Magic;
    ULONG                   af_Count;
    struct AtpFeedPost     *af_Posts;
    STRPTR                  af_Cursor;
};

struct AtpProfile
{
    ULONG                   ap_Magic;
    STRPTR                  ap_Did;
    STRPTR                  ap_Handle;
    STRPTR                  ap_DisplayName;
    STRPTR                  ap_Description;
};

/* Memory helpers */
APTR atp_alloc(ULONG size);
void atp_free(APTR p);
STRPTR atp_strdup(STRPTR s);
STRPTR atp_strndup(STRPTR s, ULONG n);
void atp_set_error(struct AmiAtpBase *base, LONG code);
void atp_conn_error(struct AtpConnection *conn, LONG code);

/* Verbose XRPC / connection diagnostics (ATPA_VERBOSE). */
void atp_verbose(struct AtpConnection *conn, STRPTR fmt, ...);
void atp_verbose_body(struct AtpConnection *conn, STRPTR label, STRPTR body,
    ULONG max_preview);

/* XRPC */
LONG atp_xrpc_request(struct AtpConnection *conn, STRPTR base_url,
    STRPTR method, STRPTR nsid, STRPTR query,
    STRPTR json_body, BOOL use_bearer, BOOL use_refresh_token,
    STRPTR *out_body, LONG *out_http_status);

LONG atp_conn_ensure_http(struct AtpConnection *conn);
LONG atp_conn_apply_session_attrs(struct AtpConnection *conn);

/* Query string builders (atp_xrpc.c) */
LONG atp_build_query2(STRPTR dest, ULONG destlen,
    STRPTR k1, STRPTR v1,
    STRPTR k2, STRPTR v2);
LONG atp_build_query3(STRPTR dest, ULONG destlen,
    STRPTR k1, STRPTR v1,
    STRPTR k2, STRPTR v2,
    STRPTR k3, STRPTR v3);

/* ISO-8601 UTC timestamp for post createdAt (atp_util.c) */
void atp_now_iso8601(struct AtpConnection *conn, STRPTR buf, ULONG buflen);
/* Parse RFC 1123 Date: into ISO-8601; store on connection when valid. */
LONG atp_conn_note_http_date(struct AtpConnection *conn, STRPTR http_date);

/* XRPC with one refresh retry on 401 */
LONG atp_xrpc_authed(struct AtpConnection *conn, STRPTR base_url,
    STRPTR method, STRPTR nsid, STRPTR query,
    STRPTR json_body, STRPTR *out_body, LONG *out_http_status);

/* Connection internals used by LVO wrappers */
struct AtpConnection *atp_new_connection(struct AmiAtpBase *base);
void atp_dispose_connection(struct AtpConnection *conn);
LONG atp_set_connection_attrs(struct AtpConnection *conn, struct TagItem *tags);
LONG atp_login(struct AtpConnection *conn, STRPTR identifier, STRPTR password);
LONG atp_refresh_session(struct AtpConnection *conn);
LONG atp_logout(struct AtpConnection *conn);

struct AtpRecord *atp_new_record(void);
void atp_dispose_record(struct AtpRecord *rec);
LONG atp_set_record_attrs(struct AtpRecord *rec, struct TagItem *tags);
LONG atp_get_record(struct AtpConnection *conn, struct AtpRecord *rec);
LONG atp_create_record(struct AtpConnection *conn, struct AtpRecord *rec);
LONG atp_put_record(struct AtpConnection *conn, struct AtpRecord *rec);
LONG atp_delete_record(struct AtpConnection *conn, struct AtpRecord *rec);
LONG atp_list_records(struct AtpConnection *conn, STRPTR collection,
    ULONG limit, STRPTR cursor, struct AtpRecordList **out_list);
void atp_dispose_record_list(struct AtpRecordList *list);

LONG atp_get_timeline(struct AtpConnection *conn, ULONG limit, STRPTR cursor,
    struct AtpFeed **out_feed);
LONG atp_get_author_feed(struct AtpConnection *conn, STRPTR actor,
    ULONG limit, STRPTR cursor, struct AtpFeed **out_feed);
void atp_dispose_feed(struct AtpFeed *feed);
LONG atp_get_profile(struct AtpConnection *conn, STRPTR actor,
    struct AtpProfile **out_profile);
void atp_dispose_profile(struct AtpProfile *profile);
LONG atp_create_post(struct AtpConnection *conn, STRPTR text,
    STRPTR uri_buf, ULONG uri_buflen);
LONG atp_resolve_handle(struct AtpConnection *conn, STRPTR handle,
    STRPTR did_buf, ULONG did_buflen);
LONG atp_download_url(struct AtpConnection *conn, STRPTR url, STRPTR path);

#endif /* ATP_PRIVATE_ATP_INTERNAL_H */
