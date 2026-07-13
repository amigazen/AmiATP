/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * atp_connection.c - AtpConnection lifecycle, attrs, login/refresh/logout
 */

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/amihttp.h>
#include <string.h>

#include "private/atp_internal.h"
#include "private/atp_json.h"
#include "private/amiatpbase.h"

extern struct Library *HttpBase;
extern struct AmiAtpBase *AtpBase;

static void
atp_conn_free_strings(struct AtpConnection *conn)
{
    atp_free(conn->ac_Service);
    atp_free(conn->ac_AppView);
    atp_free(conn->ac_CABundle);
    atp_free(conn->ac_UserAgent);
    atp_free(conn->ac_AccessJwt);
    atp_free(conn->ac_RefreshJwt);
    atp_free(conn->ac_Did);
    atp_free(conn->ac_Handle);
    conn->ac_Service = NULL;
    conn->ac_AppView = NULL;
    conn->ac_CABundle = NULL;
    conn->ac_UserAgent = NULL;
    conn->ac_AccessJwt = NULL;
    conn->ac_RefreshJwt = NULL;
    conn->ac_Did = NULL;
    conn->ac_Handle = NULL;
}

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

LONG
atp_conn_apply_session_attrs(struct AtpConnection *conn)
{
    LONG ok;

    if (conn == NULL || conn->ac_HttpSession == NULL || HttpBase == NULL) {
        return ERROR_ATP_NO_HTTP;
    }
    ok = SetHttpSessionAttrs(conn->ac_HttpSession,
        HTSA_USERAGENT, (ULONG)(conn->ac_UserAgent != NULL ?
            conn->ac_UserAgent : (STRPTR)ATP_DEFAULT_UA),
        HTSA_FOLLOW_REDIRECTS, (ULONG)TRUE,
        HTSA_KEEPALIVE, (ULONG)TRUE,
        HTSA_CONNECT_TIMEOUT, conn->ac_ConnectTimeout,
        HTSA_READ_TIMEOUT, conn->ac_ReadTimeout,
        HTSA_CA_BUNDLE_PATH, (ULONG)(conn->ac_CABundle != NULL ?
            conn->ac_CABundle : (STRPTR)""),
        TAG_DONE);
    if (!ok) {
        return ERROR_ATP_HTTP;
    }
    if (conn->ac_CABundle != NULL && conn->ac_CABundle[0] != '\0') {
        HttpBaseTags(
            HTBT_CA_BUNDLE_PATH, (ULONG)conn->ac_CABundle,
            TAG_DONE);
    }
    return 0;
}

LONG
atp_conn_ensure_http(struct AtpConnection *conn)
{
    if (conn == NULL) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (HttpBase == NULL) {
        return ERROR_ATP_NO_HTTP;
    }
    if (conn->ac_HttpSession == NULL) {
        conn->ac_HttpSession = NewHttpSession();
        if (conn->ac_HttpSession == NULL) {
            return ERROR_ATP_OUT_OF_MEMORY;
        }
        return atp_conn_apply_session_attrs(conn);
    }
    return 0;
}

struct AtpConnection *
atp_new_connection(struct AmiAtpBase *base)
{
    struct AtpConnection *conn;
    STRPTR svc;
    STRPTR app;
    STRPTR ua;
    STRPTR ca;

    conn = (struct AtpConnection *)atp_alloc(sizeof(*conn));
    if (conn == NULL) {
        atp_set_error(base, ERROR_ATP_OUT_OF_MEMORY);
        return NULL;
    }
    conn->ac_Magic = ATP_MAGIC_CONN;
    conn->ac_Base = base;
    conn->ac_ConnectTimeout = 15;
    conn->ac_ReadTimeout = 30;

    svc = ATP_DEFAULT_SERVICE;
    app = ATP_DEFAULT_APPVIEW;
    ua = ATP_DEFAULT_UA;
    ca = NULL;
    if (base != NULL) {
        if (base->aab_DefaultService != NULL) {
            svc = base->aab_DefaultService;
        }
        if (base->aab_DefaultAppView != NULL) {
            app = base->aab_DefaultAppView;
        }
        if (base->aab_DefaultUserAgent != NULL) {
            ua = base->aab_DefaultUserAgent;
        }
        if (base->aab_CABundlePath != NULL) {
            ca = base->aab_CABundlePath;
        }
    }
    if (atp_replace_str(&conn->ac_Service, svc) != 0 ||
        atp_replace_str(&conn->ac_AppView, app) != 0 ||
        atp_replace_str(&conn->ac_UserAgent, ua) != 0) {
        atp_conn_free_strings(conn);
        atp_free(conn);
        atp_set_error(base, ERROR_ATP_OUT_OF_MEMORY);
        return NULL;
    }
    if (ca != NULL) {
        if (atp_replace_str(&conn->ac_CABundle, ca) != 0) {
            atp_conn_free_strings(conn);
            atp_free(conn);
            atp_set_error(base, ERROR_ATP_OUT_OF_MEMORY);
            return NULL;
        }
    }
    return conn;
}

void
atp_dispose_connection(struct AtpConnection *conn)
{
    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return;
    }
    if (conn->ac_HttpSession != NULL && HttpBase != NULL) {
        DisposeHttpSession(conn->ac_HttpSession);
        conn->ac_HttpSession = NULL;
    }
    conn->ac_Magic = 0;
    atp_conn_free_strings(conn);
    atp_free(conn);
}

LONG
atp_set_connection_attrs(struct AtpConnection *conn, struct TagItem *tags)
{
    struct TagItem *tstate;
    struct TagItem *tag;
    LONG err;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (tags == NULL) {
        return 0;
    }
    tstate = tags;
    while ((tag = NextTagItem(&tstate)) != NULL) {
        switch (tag->ti_Tag) {
        case ATPA_SERVICE:
            err = atp_replace_str(&conn->ac_Service, (STRPTR)tag->ti_Data);
            if (err != 0) {
                atp_conn_error(conn, err);
                return err;
            }
            break;
        case ATPA_APPVIEW:
            err = atp_replace_str(&conn->ac_AppView, (STRPTR)tag->ti_Data);
            if (err != 0) {
                atp_conn_error(conn, err);
                return err;
            }
            break;
        case ATPA_CA_BUNDLE:
            err = atp_replace_str(&conn->ac_CABundle, (STRPTR)tag->ti_Data);
            if (err != 0) {
                atp_conn_error(conn, err);
                return err;
            }
            break;
        case ATPA_USERAGENT:
            err = atp_replace_str(&conn->ac_UserAgent, (STRPTR)tag->ti_Data);
            if (err != 0) {
                atp_conn_error(conn, err);
                return err;
            }
            break;
        case ATPA_CONNECT_TIMEOUT:
            conn->ac_ConnectTimeout = tag->ti_Data;
            break;
        case ATPA_READ_TIMEOUT:
            conn->ac_ReadTimeout = tag->ti_Data;
            break;
        case ATPA_VERBOSE:
            conn->ac_Verbose = tag->ti_Data ? TRUE : FALSE;
            break;
        default:
            break;
        }
    }
    if (conn->ac_HttpSession != NULL) {
        err = atp_conn_apply_session_attrs(conn);
        if (err != 0) {
            atp_conn_error(conn, err);
            return err;
        }
    }
    return 0;
}

static LONG
atp_login_parse(struct AtpConnection *conn, STRPTR body)
{
    STRPTR access;
    STRPTR refresh;
    STRPTR did;
    STRPTR handle;

    access = atp_json_object_dup_string(body, "accessJwt");
    refresh = atp_json_object_dup_string(body, "refreshJwt");
    did = atp_json_object_dup_string(body, "did");
    handle = atp_json_object_dup_string(body, "handle");
    if (access == NULL || refresh == NULL || did == NULL) {
        atp_free(access);
        atp_free(refresh);
        atp_free(did);
        atp_free(handle);
        return ERROR_ATP_JSON;
    }
    atp_free(conn->ac_AccessJwt);
    atp_free(conn->ac_RefreshJwt);
    atp_free(conn->ac_Did);
    atp_free(conn->ac_Handle);
    conn->ac_AccessJwt = access;
    conn->ac_RefreshJwt = refresh;
    conn->ac_Did = did;
    conn->ac_Handle = handle;
    conn->ac_LoggedIn = TRUE;
    atp_verbose(conn, "session ok handle=%s did=%s access_len=%lu refresh_len=%lu\n",
        handle != NULL ? handle : (STRPTR)"?",
        did != NULL ? did : (STRPTR)"?",
        (unsigned long)strlen((char *)access),
        (unsigned long)strlen((char *)refresh));
    return 0;
}

LONG
atp_login(struct AtpConnection *conn, STRPTR identifier, STRPTR password)
{
    STRPTR body_in;
    STRPTR body_out;
    LONG http_status;
    LONG err;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (identifier == NULL || password == NULL) {
        atp_conn_error(conn, ERROR_ATP_INVALID_ARG);
        return ERROR_ATP_INVALID_ARG;
    }
    body_in = atp_json_build_create_session(identifier, password);
    if (body_in == NULL) {
        atp_conn_error(conn, ERROR_ATP_OUT_OF_MEMORY);
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    body_out = NULL;
    err = atp_xrpc_request(conn, conn->ac_Service, "POST",
        "com.atproto.server.createSession", NULL, body_in,
        FALSE, FALSE, &body_out, &http_status);
    atp_free(body_in);
    if (err != 0) {
        atp_free(body_out);
        atp_conn_error(conn, err);
        return err;
    }
    err = atp_login_parse(conn, body_out);
    atp_free(body_out);
    if (err != 0) {
        atp_conn_error(conn, err);
        return err;
    }
    atp_conn_error(conn, 0);
    return 0;
}

LONG
atp_refresh_session(struct AtpConnection *conn)
{
    STRPTR body_out;
    LONG http_status;
    LONG err;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (conn->ac_RefreshJwt == NULL) {
        atp_conn_error(conn, ERROR_ATP_NOT_LOGGED_IN);
        return ERROR_ATP_NOT_LOGGED_IN;
    }
    body_out = NULL;
    err = atp_xrpc_request(conn, conn->ac_Service, "POST",
        "com.atproto.server.refreshSession", NULL, NULL,
        TRUE, TRUE, &body_out, &http_status);
    if (err != 0) {
        atp_free(body_out);
        atp_conn_error(conn, err);
        return err;
    }
    err = atp_login_parse(conn, body_out);
    atp_free(body_out);
    if (err != 0) {
        atp_conn_error(conn, err);
        return err;
    }
    atp_conn_error(conn, 0);
    return 0;
}

LONG
atp_logout(struct AtpConnection *conn)
{
    STRPTR body_out;
    LONG http_status;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN) {
        return ERROR_ATP_INVALID_HANDLE;
    }
    if (conn->ac_AccessJwt != NULL) {
        body_out = NULL;
        (void)atp_xrpc_request(conn, conn->ac_Service, "POST",
            "com.atproto.server.deleteSession", NULL, NULL,
            TRUE, FALSE, &body_out, &http_status);
        atp_free(body_out);
    }
    atp_free(conn->ac_AccessJwt);
    atp_free(conn->ac_RefreshJwt);
    atp_free(conn->ac_Did);
    atp_free(conn->ac_Handle);
    conn->ac_AccessJwt = NULL;
    conn->ac_RefreshJwt = NULL;
    conn->ac_Did = NULL;
    conn->ac_Handle = NULL;
    conn->ac_LoggedIn = FALSE;
    atp_conn_error(conn, 0);
    return 0;
}

/*
 * Retry once after refresh when XRPC returns ERROR_ATP_AUTH.
 */
LONG
atp_xrpc_authed(struct AtpConnection *conn, STRPTR base_url,
    STRPTR method, STRPTR nsid, STRPTR query,
    STRPTR json_body, STRPTR *out_body, LONG *out_http_status)
{
    LONG err;

    err = atp_xrpc_request(conn, base_url, method, nsid, query, json_body,
        TRUE, FALSE, out_body, out_http_status);
    if (err != ERROR_ATP_AUTH) {
        return err;
    }
    atp_free(*out_body);
    *out_body = NULL;
    err = atp_refresh_session(conn);
    if (err != 0) {
        return err;
    }
    return atp_xrpc_request(conn, base_url, method, nsid, query, json_body,
        TRUE, FALSE, out_body, out_http_status);
}
