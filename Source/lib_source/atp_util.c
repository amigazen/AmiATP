/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * atp_util.c - Allocation, errors, verbose logging for amiatp.library
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <utility/date.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "private/atp_internal.h"

APTR
atp_alloc(ULONG size)
{
    APTR p;

    if (size == 0) {
        return NULL;
    }
    p = AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
    return p;
}

void
atp_free(APTR p)
{
    if (p != NULL) {
        FreeVec(p);
    }
}

STRPTR
atp_strdup(STRPTR s)
{
    ULONG n;
    STRPTR d;

    if (s == NULL) {
        return NULL;
    }
    n = (ULONG)strlen((char *)s);
    d = (STRPTR)atp_alloc(n + 1);
    if (d == NULL) {
        return NULL;
    }
    CopyMem((APTR)s, d, n + 1);
    return d;
}

STRPTR
atp_strndup(STRPTR s, ULONG n)
{
    ULONG i;
    STRPTR d;

    if (s == NULL) {
        return NULL;
    }
    i = 0;
    while (i < n && s[i] != '\0') {
        i++;
    }
    d = (STRPTR)atp_alloc(i + 1);
    if (d == NULL) {
        return NULL;
    }
    if (i > 0) {
        CopyMem((APTR)s, d, i);
    }
    d[i] = '\0';
    return d;
}

void
atp_set_error(struct AmiAtpBase *base, LONG code)
{
    if (base != NULL) {
        base->aab_LastError = code;
    }
}

void
atp_conn_error(struct AtpConnection *conn, LONG code)
{
    if (conn != NULL) {
        conn->ac_LastError = code;
        if (conn->ac_Base != NULL) {
            conn->ac_Base->aab_LastError = code;
        }
    }
}

void
atp_verbose(struct AtpConnection *conn, STRPTR fmt, ...)
{
    UBYTE buf[384];
    va_list ap;

    if (conn == NULL || !conn->ac_Verbose || fmt == NULL) {
        return;
    }
    va_start(ap, fmt);
    vsprintf((char *)buf, (char *)fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = '\0';
    PutStr("amiatp: ");
    PutStr((STRPTR)buf);
}

void
atp_verbose_body(struct AtpConnection *conn, STRPTR label, STRPTR body,
    ULONG max_preview)
{
    ULONG len;
    ULONG show;
    ULONG i;
    UBYTE ch[2];

    if (conn == NULL || !conn->ac_Verbose) {
        return;
    }
    if (label == NULL) {
        label = (STRPTR)"body";
    }
    if (body == NULL) {
        Printf("amiatp: %s: (null)\n", label);
        return;
    }
    len = (ULONG)strlen((char *)body);
    show = len;
    if (max_preview > 0 && show > max_preview) {
        show = max_preview;
    }
    Printf("amiatp: %s: %lu bytes\n", label, (unsigned long)len);
    PutStr("amiatp: | ");
    ch[1] = '\0';
    for (i = 0; i < show; i++) {
        UBYTE c;

        c = (UBYTE)body[i];
        if (c == '\n' || c == '\r') {
            PutStr("\\n");
        } else if (c < 0x20 || c > 0x7e) {
            Printf("\\x%02lx", (unsigned long)c);
        } else {
            ch[0] = c;
            PutStr((STRPTR)ch);
        }
    }
    if (show < len) {
        PutStr("...");
    }
    PutStr("\n");
}

void
atp_now_iso8601(struct AtpConnection *conn, STRPTR buf, ULONG buflen)
{
    struct DateStamp ds;
    struct ClockData cd;
    ULONG minutes;
    LONG second;

    if (buf == NULL || buflen < 25) {
        return;
    }

    /*
     * Prefer HTTP Date captured from prior XRPC (login etc.).  Classic Amigas
     * without an RTC often sit on the 1978 epoch — Bluesky then archives the
     * post as ancient.
     */
    if (conn != NULL && conn->ac_NetworkIso8601[0] != '\0') {
        strncpy((char *)buf, (char *)conn->ac_NetworkIso8601,
            (size_t)(buflen - 1));
        buf[buflen - 1] = '\0';
        return;
    }

    DateStamp(&ds);
    minutes = (ULONG)ds.ds_Days * 1440UL + (ULONG)ds.ds_Minute;
    Amiga2Date(minutes, &cd);
    second = (LONG)(ds.ds_Tick / 50);
    if (second > 59) {
        second = 59;
    }
    /* Refuse obviously unset clocks (pre-Y2K) — force a sentinel callers notice. */
    if (cd.year < 2000) {
        strcpy((char *)buf, "1970-01-01T00:00:00.000Z");
        return;
    }
    sprintf((char *)buf, "%04ld-%02ld-%02ldT%02ld:%02ld:%02ld.000Z",
        (LONG)cd.year, (LONG)cd.month, (LONG)cd.mday,
        (LONG)cd.hour, (LONG)cd.min, second);
}

static LONG
atp_month_from_name(STRPTR mon)
{
    static const char *names[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    LONG i;

    if (mon == NULL) {
        return 0;
    }
    for (i = 0; i < 12; i++) {
        if (mon[0] == names[i][0] && mon[1] == names[i][1] &&
            mon[2] == names[i][2]) {
            return i + 1;
        }
    }
    return 0;
}

LONG
atp_conn_note_http_date(struct AtpConnection *conn, STRPTR http_date)
{
    LONG day;
    LONG year;
    LONG hour;
    LONG min;
    LONG sec;
    LONG month;
    char mon[8];
    int n;

    if (conn == NULL || http_date == NULL || http_date[0] == '\0') {
        return 0;
    }

    /*
     * RFC 1123: "Wed, 13 Jul 2026 10:55:00 GMT"
     * Also accept without weekday: "13 Jul 2026 10:55:00 GMT"
     */
    mon[0] = '\0';
    n = sscanf((char *)http_date, "%*3s, %ld %3s %ld %ld:%ld:%ld",
        &day, mon, &year, &hour, &min, &sec);
    if (n < 6) {
        n = sscanf((char *)http_date, "%ld %3s %ld %ld:%ld:%ld",
            &day, mon, &year, &hour, &min, &sec);
    }
    if (n < 6) {
        atp_verbose(conn, "Date header parse failed: %s\n", http_date);
        return 0;
    }
    month = atp_month_from_name((STRPTR)mon);
    if (month < 1 || year < 2000 || day < 1 || day > 31) {
        atp_verbose(conn, "Date header rejected: %s\n", http_date);
        return 0;
    }
    sprintf((char *)conn->ac_NetworkIso8601,
        "%04ld-%02ld-%02ldT%02ld:%02ld:%02ld.000Z",
        year, month, day, hour, min, sec);
    atp_verbose(conn, "network time from Date: %s\n",
        conn->ac_NetworkIso8601);
    return 1;
}
