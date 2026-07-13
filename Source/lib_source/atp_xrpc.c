/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * atp_xrpc.c - XRPC GET/POST over amihttp.library Tier 2
 */

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/amihttp.h>
#include <string.h>
#include <stdio.h>

#include "private/atp_internal.h"
#include "private/atp_json.h"

extern struct Library *HttpBase;

static LONG
atp_xrpc_read_all(struct HttpTransaction *txn, STRPTR *out_body)
{
    UBYTE chunk[4096];
    STRPTR buf;
    ULONG cap;
    ULONG used;
    LONG n;

    *out_body = NULL;
    buf = NULL;
    cap = 0;
    used = 0;

    for (;;) {
        n = HttpTransactionReadBody(txn, chunk, sizeof(chunk));
        if (n < 0) {
            atp_free(buf);
            return ERROR_ATP_HTTP;
        }
        if (n == 0) {
            break;
        }
        if (used + (ULONG)n + 1 > ATP_MAX_BODY) {
            atp_free(buf);
            return ERROR_ATP_BUFFER_TOO_SMALL;
        }
        if (used + (ULONG)n + 1 > cap) {
            ULONG ncap;
            STRPTR nbuf;

            ncap = cap == 0 ? 8192UL : cap * 2UL;
            while (ncap < used + (ULONG)n + 1) {
                ncap *= 2UL;
            }
            if (ncap > ATP_MAX_BODY) {
                ncap = ATP_MAX_BODY;
            }
            nbuf = (STRPTR)atp_alloc(ncap);
            if (nbuf == NULL) {
                atp_free(buf);
                return ERROR_ATP_OUT_OF_MEMORY;
            }
            if (buf != NULL && used > 0) {
                CopyMem(buf, nbuf, used);
                atp_free(buf);
            }
            buf = nbuf;
            cap = ncap;
        }
        CopyMem(chunk, buf + used, (ULONG)n);
        used += (ULONG)n;
        buf[used] = '\0';
    }

    if (buf == NULL) {
        buf = atp_strdup("");
        if (buf == NULL) {
            return ERROR_ATP_OUT_OF_MEMORY;
        }
    }
    *out_body = buf;
    return 0;
}

static LONG
atp_url_encode_query_append(STRPTR dest, ULONG destlen, ULONG *used,
    STRPTR key, STRPTR value)
{
    ULONG u;
    STRPTR p;
    char hex[4];

    if (dest == NULL || used == NULL || key == NULL || value == NULL) {
        return 0;
    }
    u = *used;
    if (u > 0) {
        if (u + 1 >= destlen) {
            return 0;
        }
        dest[u++] = '&';
    }
    p = key;
    while (*p != '\0') {
        if (u + 1 >= destlen) {
            return 0;
        }
        dest[u++] = *p++;
    }
    if (u + 1 >= destlen) {
        return 0;
    }
    dest[u++] = '=';
    p = value;
    while (*p != '\0') {
        UBYTE c;

        c = (UBYTE)*p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            if (u + 1 >= destlen) {
                return 0;
            }
            dest[u++] = (char)c;
        } else {
            if (u + 3 >= destlen) {
                return 0;
            }
            sprintf(hex, "%%%02X", (unsigned)c);
            dest[u++] = hex[0];
            dest[u++] = hex[1];
            dest[u++] = hex[2];
        }
        p++;
    }
    dest[u] = '\0';
    *used = u;
    return 1;
}

static LONG
atp_xrpc_json_error(STRPTR body)
{
    UBYTE errname[64];
    UBYTE msg[192];

    if (body == NULL || body[0] == '\0') {
        return 0;
    }
    if (!atp_json_object_get_string(body, "error", (STRPTR)errname,
            sizeof(errname))) {
        return 0;
    }
    if (errname[0] == '\0') {
        return 0;
    }
    msg[0] = '\0';
    (void)atp_json_object_get_string(body, "message", (STRPTR)msg, sizeof(msg));
    return 1;
}

LONG
atp_xrpc_request(struct AtpConnection *conn, STRPTR base_url,
    STRPTR method, STRPTR nsid, STRPTR query,
    STRPTR json_body, BOOL use_bearer, BOOL use_refresh_token,
    STRPTR *out_body, LONG *out_http_status)
{
    struct HttpTransaction *txn;
    UBYTE url[ATP_MAX_URL];
    STRPTR auth;
    STRPTR token;
    LONG status;
    LONG err;
    LONG ok;
    ULONG token_len;

    auth = NULL;
    if (out_body != NULL) {
        *out_body = NULL;
    }
    if (out_http_status != NULL) {
        *out_http_status = 0;
    }
    if (conn == NULL || base_url == NULL || method == NULL || nsid == NULL) {
        return ERROR_ATP_INVALID_ARG;
    }
    if (HttpBase == NULL) {
        return ERROR_ATP_NO_HTTP;
    }

    err = atp_conn_ensure_http(conn);
    if (err != 0) {
        atp_verbose(conn, "ensure_http failed err=%ld\n", err);
        return err;
    }

    if (strlen(base_url) + strlen(nsid) + 16 >= sizeof(url)) {
        return ERROR_ATP_BUFFER_TOO_SMALL;
    }
    strcpy((char *)url, (char *)base_url);
    {
        ULONG bl;

        bl = (ULONG)strlen((char *)url);
        while (bl > 0 && url[bl - 1] == '/') {
            url[--bl] = '\0';
        }
    }
    strcat((char *)url, "/xrpc/");
    strcat((char *)url, (char *)nsid);
    if (query != NULL && query[0] != '\0') {
        if (strlen((char *)url) + strlen(query) + 2 >= sizeof(url)) {
            return ERROR_ATP_BUFFER_TOO_SMALL;
        }
        strcat((char *)url, "?");
        strcat((char *)url, (char *)query);
    }

    atp_verbose(conn, "XRPC %s %s\n", method, url);
    atp_verbose(conn, "  bearer=%s refresh_tok=%s body_len=%lu\n",
        use_bearer ? (STRPTR)"yes" : (STRPTR)"no",
        use_refresh_token ? (STRPTR)"yes" : (STRPTR)"no",
        (unsigned long)(json_body != NULL ? strlen(json_body) : 0));
    if (json_body != NULL) {
        atp_verbose_body(conn, "request", json_body, 400);
    }

    txn = NewHttpTransaction(conn->ac_HttpSession);
    if (txn == NULL) {
        return ERROR_ATP_OUT_OF_MEMORY;
    }

    if (json_body != NULL) {
        ok = SetHttpTransactionAttrs(txn,
            HTTA_URL, (ULONG)url,
            HTTA_METHOD, (ULONG)method,
            HTTA_REQUEST_BODY, (ULONG)json_body,
            HTTA_POST_LENGTH, (ULONG)strlen(json_body),
            HTTA_CONTENT_TYPE, (ULONG)"application/json",
            TAG_DONE);
    } else {
        ok = SetHttpTransactionAttrs(txn,
            HTTA_URL, (ULONG)url,
            HTTA_METHOD, (ULONG)method,
            TAG_DONE);
    }
    if (!ok) {
        DisposeHttpTransaction(txn);
        atp_verbose(conn, "SetHttpTransactionAttrs failed\n");
        return ERROR_ATP_HTTP;
    }

    HttpTransactionAddHeader(txn, (STRPTR)"Accept", (STRPTR)"application/json");

    if (use_bearer) {
        token = use_refresh_token ? conn->ac_RefreshJwt : conn->ac_AccessJwt;
        if (token == NULL || token[0] == '\0') {
            DisposeHttpTransaction(txn);
            atp_verbose(conn, "missing JWT (refresh=%ld)\n",
                (LONG)use_refresh_token);
            return ERROR_ATP_NOT_LOGGED_IN;
        }
        token_len = (ULONG)strlen((char *)token);
        {
            UBYTE pref[16];
            ULONG pl;

            pl = token_len;
            if (pl > 12) {
                pl = 12;
            }
            CopyMem(token, pref, pl);
            pref[pl] = '\0';
            atp_verbose(conn, "  jwt_len=%lu prefix=%s...\n",
                (unsigned long)token_len, pref);
        }
        /* Heap — never put multi-KB JWTs on the Amiga stack. */
        auth = (STRPTR)atp_alloc(token_len + 16);
        if (auth == NULL) {
            DisposeHttpTransaction(txn);
            return ERROR_ATP_OUT_OF_MEMORY;
        }
        strcpy((char *)auth, "Bearer ");
        strcat((char *)auth, (char *)token);
        if (!HttpTransactionAddHeader(txn, (STRPTR)"Authorization", auth)) {
            atp_free(auth);
            DisposeHttpTransaction(txn);
            atp_verbose(conn, "AddHeader Authorization failed\n");
            return ERROR_ATP_HTTP;
        }
    }

    if (!HttpTransactionPerform(txn)) {
        err = HttpTransactionGetLastError(txn);
        atp_verbose(conn, "Perform FAILED HttpError=%ld\n", err);
        atp_free(auth);
        DisposeHttpTransaction(txn);
        return ERROR_ATP_HTTP;
    }

    status = HttpTransactionGetStatusCode(txn);
    if (out_http_status != NULL) {
        *out_http_status = status;
    }
    atp_verbose(conn, "  HTTP status=%ld\n", status);

    {
        STRPTR date_hdr;

        date_hdr = HttpTransactionRespHeader(txn, (STRPTR)"Date");
        if (date_hdr != NULL) {
            atp_conn_note_http_date(conn, date_hdr);
        }
    }

    err = atp_xrpc_read_all(txn, out_body);
    atp_free(auth);
    DisposeHttpTransaction(txn);
    if (err != 0) {
        atp_verbose(conn, "ReadBody failed err=%ld\n", err);
        return err;
    }
    if (out_body != NULL) {
        atp_verbose_body(conn, "response", *out_body, 500);
    }

    if (status == 401) {
        return ERROR_ATP_AUTH;
    }
    if (status == 404) {
        return ERROR_ATP_NOT_FOUND;
    }
    if (status < 200 || status >= 300) {
        atp_verbose(conn, "non-2xx status -> ERROR_ATP_HTTP_STATUS\n");
        return ERROR_ATP_HTTP_STATUS;
    }
    if (out_body != NULL && *out_body != NULL && atp_xrpc_json_error(*out_body)) {
        UBYTE errname[64];
        UBYTE msg[192];

        errname[0] = '\0';
        msg[0] = '\0';
        (void)atp_json_object_get_string(*out_body, "error",
            (STRPTR)errname, sizeof(errname));
        (void)atp_json_object_get_string(*out_body, "message",
            (STRPTR)msg, sizeof(msg));
        atp_verbose(conn, "XRPC error object: %s — %s\n", errname, msg);
        if (strcmp((char *)errname, "AuthenticationRequired") == 0 ||
            strcmp((char *)errname, "InvalidToken") == 0 ||
            strcmp((char *)errname, "ExpiredToken") == 0) {
            return ERROR_ATP_AUTH;
        }
        return ERROR_ATP_XRPC;
    }
    return 0;
}

LONG
atp_build_query2(STRPTR dest, ULONG destlen,
    STRPTR k1, STRPTR v1,
    STRPTR k2, STRPTR v2)
{
    ULONG used;

    if (dest == NULL || destlen < 1) {
        return 0;
    }
    dest[0] = '\0';
    used = 0;
    if (k1 != NULL && v1 != NULL) {
        if (!atp_url_encode_query_append(dest, destlen, &used, k1, v1)) {
            return 0;
        }
    }
    if (k2 != NULL && v2 != NULL) {
        if (!atp_url_encode_query_append(dest, destlen, &used, k2, v2)) {
            return 0;
        }
    }
    return 1;
}

LONG
atp_build_query3(STRPTR dest, ULONG destlen,
    STRPTR k1, STRPTR v1,
    STRPTR k2, STRPTR v2,
    STRPTR k3, STRPTR v3)
{
    ULONG used;

    if (dest == NULL || destlen < 1) {
        return 0;
    }
    dest[0] = '\0';
    used = 0;
    if (k1 != NULL && v1 != NULL) {
        if (!atp_url_encode_query_append(dest, destlen, &used, k1, v1)) {
            return 0;
        }
    }
    if (k2 != NULL && v2 != NULL) {
        if (!atp_url_encode_query_append(dest, destlen, &used, k2, v2)) {
            return 0;
        }
    }
    if (k3 != NULL && v3 != NULL) {
        if (!atp_url_encode_query_append(dest, destlen, &used, k3, v3)) {
            return 0;
        }
    }
    return 1;
}

/*
 * Download an absolute URL (e.g. CDN thumb) to a DOS path using the
 * connection's HttpSession (same CA / TLS settings as XRPC).
 */
LONG
atp_download_url(struct AtpConnection *conn, STRPTR url, STRPTR path)
{
    struct HttpTransaction *txn;
    BPTR fh;
    UBYTE chunk[4096];
    ULONG total;
    LONG n;
    LONG status;
    LONG err;
    LONG ok;

    if (conn == NULL || conn->ac_Magic != ATP_MAGIC_CONN ||
        url == NULL || url[0] == '\0' || path == NULL || path[0] == '\0') {
        return ERROR_ATP_INVALID_ARG;
    }
    if (HttpBase == NULL) {
        return ERROR_ATP_NO_HTTP;
    }
    err = atp_conn_ensure_http(conn);
    if (err != 0) {
        return err;
    }

    atp_verbose(conn, "DOWNLOAD %s -> %s\n", url, path);

    txn = NewHttpTransaction(conn->ac_HttpSession);
    if (txn == NULL) {
        return ERROR_ATP_OUT_OF_MEMORY;
    }
    ok = SetHttpTransactionAttrs(txn,
        HTTA_URL, (ULONG)url,
        HTTA_METHOD, (ULONG)"GET",
        TAG_DONE);
    if (!ok) {
        DisposeHttpTransaction(txn);
        return ERROR_ATP_HTTP;
    }

    if (!HttpTransactionPerform(txn)) {
        err = HttpTransactionGetLastError(txn);
        DisposeHttpTransaction(txn);
        atp_verbose(conn, "DOWNLOAD Perform failed HttpError=%ld\n", err);
        return ERROR_ATP_HTTP;
    }

    status = HttpTransactionGetStatusCode(txn);
    atp_verbose(conn, "  DOWNLOAD HTTP status=%ld\n", status);
    if (status < 200 || status >= 300) {
        DisposeHttpTransaction(txn);
        return ERROR_ATP_HTTP_STATUS;
    }

    fh = Open(path, MODE_NEWFILE);
    if (fh == 0) {
        DisposeHttpTransaction(txn);
        return ERROR_ATP_HTTP;
    }

    total = 0;
    err = 0;
    for (;;) {
        n = HttpTransactionReadBody(txn, chunk, sizeof(chunk));
        if (n < 0) {
            err = ERROR_ATP_HTTP;
            break;
        }
        if (n == 0) {
            break;
        }
        total += (ULONG)n;
        if (total > ATP_MAX_DOWNLOAD) {
            err = ERROR_ATP_BUFFER_TOO_SMALL;
            break;
        }
        if (Write(fh, chunk, n) != n) {
            err = ERROR_ATP_HTTP;
            break;
        }
    }
    Close(fh);
    DisposeHttpTransaction(txn);
    if (err != 0) {
        DeleteFile(path);
        return err;
    }
    atp_verbose(conn, "  DOWNLOAD ok bytes=%lu\n", (unsigned long)total);
    return 0;
}
