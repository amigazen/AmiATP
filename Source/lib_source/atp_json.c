/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * atp_json.c - Private AT-scoped JSON extract/build (not exported)
 *
 * Enough to drive XRPC session and Bluesky Lexicon shapes without a
 * general-purpose JSON library.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

#include "private/atp_json.h"
#include "private/atp_internal.h"

STRPTR
atp_json_skip(STRPTR p)
{
    if (p == NULL) {
        return NULL;
    }
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    return p;
}

static STRPTR
atp_json_skip_string(STRPTR p)
{
    if (p == NULL || *p != '"') {
        return p;
    }
    p++;
    while (*p != '\0') {
        if (*p == '\\' && p[1] != '\0') {
            p += 2;
            continue;
        }
        if (*p == '"') {
            return p + 1;
        }
        p++;
    }
    return p;
}

static STRPTR
atp_json_skip_value(STRPTR p)
{
    LONG depth;

    p = atp_json_skip(p);
    if (p == NULL || *p == '\0') {
        return p;
    }
    if (*p == '"') {
        return atp_json_skip_string(p);
    }
    if (*p == '{' || *p == '[') {
        depth = 0;
        do {
            if (*p == '"' ) {
                p = atp_json_skip_string(p);
                continue;
            }
            if (*p == '{' || *p == '[') {
                depth++;
            } else if (*p == '}' || *p == ']') {
                depth--;
            }
            if (*p == '\0') {
                return p;
            }
            p++;
        } while (depth > 0);
        return p;
    }
    while (*p != '\0' && *p != ',' && *p != '}' && *p != ']' &&
           *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        p++;
    }
    return p;
}

static int
atp_json_key_match(STRPTR p, STRPTR key)
{
    ULONG klen;

    if (p == NULL || key == NULL || *p != '"') {
        return 0;
    }
    klen = (ULONG)strlen(key);
    p++;
    if (strncmp(p, key, (size_t)klen) != 0) {
        return 0;
    }
    if (p[klen] != '"') {
        return 0;
    }
    return 1;
}

STRPTR
atp_json_find_key(STRPTR json, STRPTR key)
{
    STRPTR p;
    LONG depth;

    if (json == NULL || key == NULL) {
        return NULL;
    }
    p = atp_json_skip(json);
    if (*p != '{') {
        return NULL;
    }
    p++;
    depth = 1;
    while (*p != '\0' && depth > 0) {
        p = atp_json_skip(p);
        if (*p == '}') {
            depth--;
            p++;
            continue;
        }
        if (*p == '{') {
            depth++;
            p++;
            continue;
        }
        if (*p == '"') {
            if (depth == 1 && atp_json_key_match(p, key)) {
                p = atp_json_skip_string(p);
                p = atp_json_skip(p);
                if (*p == ':') {
                    p++;
                    return atp_json_skip(p);
                }
                return NULL;
            }
            p = atp_json_skip_string(p);
            p = atp_json_skip(p);
            if (*p == ':') {
                p++;
                p = atp_json_skip_value(atp_json_skip(p));
            }
            p = atp_json_skip(p);
            if (*p == ',') {
                p++;
            }
            continue;
        }
        p++;
    }
    return NULL;
}

LONG
atp_json_get_string(STRPTR p, STRPTR buf, ULONG buflen)
{
    ULONG o;

    if (p == NULL || buf == NULL || buflen < 1) {
        return 0;
    }
    p = atp_json_skip(p);
    if (*p != '"') {
        return 0;
    }
    p++;
    o = 0;
    while (*p != '\0' && *p != '"') {
        if (*p == '\\' && p[1] != '\0') {
            p++;
            if (o + 1 >= buflen) {
                buf[buflen - 1] = '\0';
                return 0;
            }
            switch (*p) {
            case 'n': buf[o++] = '\n'; break;
            case 'r': buf[o++] = '\r'; break;
            case 't': buf[o++] = '\t'; break;
            case '"': buf[o++] = '"'; break;
            case '\\': buf[o++] = '\\'; break;
            case '/': buf[o++] = '/'; break;
            case 'u':
                /* Skip \uXXXX; store '?' for Amiga Latin-1 safety. */
                if (p[1] && p[2] && p[3] && p[4]) {
                    p += 4;
                }
                buf[o++] = '?';
                break;
            default:
                buf[o++] = *p;
                break;
            }
            p++;
            continue;
        }
        if (o + 1 >= buflen) {
            buf[buflen - 1] = '\0';
            return 0;
        }
        buf[o++] = *p++;
    }
    buf[o] = '\0';
    return 1;
}

LONG
atp_json_object_get_string(STRPTR json, STRPTR key,
    STRPTR buf, ULONG buflen)
{
    STRPTR v;

    v = atp_json_find_key(json, key);
    if (v == NULL) {
        return 0;
    }
    return atp_json_get_string(v, buf, buflen);
}

STRPTR
atp_json_object_dup_string(STRPTR json, STRPTR key)
{
    STRPTR tmp;
    STRPTR v;
    STRPTR out;

    /*
     * Heap buffer — JWTs exceed safe Amiga stack frames if kept automatic.
     */
    tmp = (STRPTR)atp_alloc(ATP_MAX_JWT);
    if (tmp == NULL) {
        return NULL;
    }
    v = atp_json_find_key(json, key);
    if (v == NULL) {
        atp_free(tmp);
        return NULL;
    }
    if (!atp_json_get_string(v, tmp, ATP_MAX_JWT)) {
        atp_free(tmp);
        return NULL;
    }
    out = atp_strdup(tmp);
    atp_free(tmp);
    return out;
}

LONG
atp_json_object_get_long(STRPTR json, STRPTR key, LONG *out)
{
    STRPTR v;
    LONG sign;
    LONG n;

    if (out == NULL) {
        return 0;
    }
    v = atp_json_find_key(json, key);
    if (v == NULL) {
        return 0;
    }
    v = atp_json_skip(v);
    sign = 1;
    if (*v == '-') {
        sign = -1;
        v++;
    }
    if (*v < '0' || *v > '9') {
        return 0;
    }
    n = 0;
    while (*v >= '0' && *v <= '9') {
        n = n * 10 + (*v - '0');
        v++;
    }
    *out = n * sign;
    return 1;
}

static STRPTR
atp_json_find_array(STRPTR json, STRPTR array_key)
{
    STRPTR v;

    v = atp_json_find_key(json, array_key);
    if (v == NULL) {
        return NULL;
    }
    v = atp_json_skip(v);
    if (*v != '[') {
        return NULL;
    }
    return v;
}

ULONG
atp_json_array_count(STRPTR json, STRPTR array_key)
{
    STRPTR p;
    ULONG count;
    LONG depth;

    p = atp_json_find_array(json, array_key);
    if (p == NULL) {
        return 0;
    }
    p++;
    count = 0;
    depth = 1;
    p = atp_json_skip(p);
    if (*p == ']') {
        return 0;
    }
    while (*p != '\0' && depth > 0) {
        if (depth == 1) {
            count++;
            p = atp_json_skip_value(p);
            p = atp_json_skip(p);
            if (*p == ',') {
                p++;
                p = atp_json_skip(p);
                continue;
            }
            if (*p == ']') {
                break;
            }
        } else {
            p++;
        }
    }
    return count;
}

LONG
atp_json_array_nth(STRPTR json, STRPTR array_key,
    ULONG index, STRPTR *elem_start, STRPTR *elem_end)
{
    STRPTR p;
    STRPTR start;
    ULONG i;

    if (elem_start == NULL || elem_end == NULL) {
        return 0;
    }
    p = atp_json_find_array(json, array_key);
    if (p == NULL) {
        return 0;
    }
    p++;
    i = 0;
    while (*p != '\0') {
        p = atp_json_skip(p);
        if (*p == ']') {
            return 0;
        }
        start = p;
        p = atp_json_skip_value(p);
        if (i == index) {
            *elem_start = start;
            *elem_end = p;
            return 1;
        }
        i++;
        p = atp_json_skip(p);
        if (*p == ',') {
            p++;
        }
    }
    return 0;
}

STRPTR
atp_json_fragment_dup_string(STRPTR start, STRPTR end,
    STRPTR key)
{
    ULONG len;
    STRPTR tmp;
    STRPTR result;

    if (start == NULL || end == NULL || end < start) {
        return NULL;
    }
    len = (ULONG)(end - start);
    tmp = (STRPTR)atp_alloc(len + 1);
    if (tmp == NULL) {
        return NULL;
    }
    CopyMem((APTR)start, tmp, len);
    tmp[len] = '\0';
    result = atp_json_object_dup_string(tmp, key);
    atp_free(tmp);
    return result;
}

STRPTR
atp_json_object_dup_string2(STRPTR json,
    STRPTR key, STRPTR subkey)
{
    STRPTR v;
    STRPTR end;

    v = atp_json_find_key(json, key);
    if (v == NULL) {
        return NULL;
    }
    v = atp_json_skip(v);
    if (*v != '{') {
        return NULL;
    }
    end = atp_json_skip_value(v);
    return atp_json_fragment_dup_string(v, end, subkey);
}

STRPTR
atp_json_fragment_dup_string2(STRPTR start, STRPTR end,
    STRPTR key, STRPTR subkey)
{
    ULONG len;
    STRPTR tmp;
    STRPTR result;

    if (start == NULL || end == NULL || end < start) {
        return NULL;
    }
    len = (ULONG)(end - start);
    tmp = (STRPTR)atp_alloc(len + 1);
    if (tmp == NULL) {
        return NULL;
    }
    CopyMem((APTR)start, tmp, len);
    tmp[len] = '\0';
    result = atp_json_object_dup_string2(tmp, key, subkey);
    atp_free(tmp);
    return result;
}

LONG
atp_json_append_escaped(STRPTR dest, ULONG destlen, ULONG *used,
    STRPTR s)
{
    ULONG u;

    if (dest == NULL || used == NULL || s == NULL) {
        return 0;
    }
    u = *used;
    if (u + 1 >= destlen) {
        return 0;
    }
    dest[u++] = '"';
    while (*s != '\0') {
        if (*s == '"' || *s == '\\') {
            if (u + 2 >= destlen) {
                return 0;
            }
            dest[u++] = '\\';
            dest[u++] = *s;
        } else if ((UBYTE)*s < 0x20) {
            if (u + 2 >= destlen) {
                return 0;
            }
            dest[u++] = ' ';
        } else {
            if (u + 1 >= destlen) {
                return 0;
            }
            dest[u++] = *s;
        }
        s++;
    }
    if (u + 1 >= destlen) {
        return 0;
    }
    dest[u++] = '"';
    dest[u] = '\0';
    *used = u;
    return 1;
}

STRPTR
atp_json_build_create_session(STRPTR identifier, STRPTR password)
{
    UBYTE buf[1024];
    ULONG used;

    if (identifier == NULL || password == NULL) {
        return NULL;
    }
    used = 0;
    strcpy((char *)buf, "{\"identifier\":");
    used = (ULONG)strlen((char *)buf);
    if (!atp_json_append_escaped((STRPTR)buf, sizeof(buf), &used, identifier)) {
        return NULL;
    }
    if (used + 14 >= sizeof(buf)) {
        return NULL;
    }
    strcpy((char *)buf + used, ",\"password\":");
    used = (ULONG)strlen((char *)buf);
    if (!atp_json_append_escaped((STRPTR)buf, sizeof(buf), &used, password)) {
        return NULL;
    }
    if (used + 2 >= sizeof(buf)) {
        return NULL;
    }
    buf[used++] = '}';
    buf[used] = '\0';
    return atp_strdup((STRPTR)buf);
}

STRPTR
atp_json_build_post_record(STRPTR text, STRPTR created_at)
{
    UBYTE buf[2048];
    ULONG used;

    if (text == NULL || created_at == NULL) {
        return NULL;
    }
    strcpy((char *)buf, "{\"$type\":\"app.bsky.feed.post\",\"text\":");
    used = (ULONG)strlen((char *)buf);
    if (!atp_json_append_escaped((STRPTR)buf, sizeof(buf), &used, text)) {
        return NULL;
    }
    if (used + 16 >= sizeof(buf)) {
        return NULL;
    }
    strcpy((char *)buf + used, ",\"createdAt\":");
    used = (ULONG)strlen((char *)buf);
    if (!atp_json_append_escaped((STRPTR)buf, sizeof(buf), &used, created_at)) {
        return NULL;
    }
    if (used + 2 >= sizeof(buf)) {
        return NULL;
    }
    buf[used++] = '}';
    buf[used] = '\0';
    return atp_strdup((STRPTR)buf);
}

STRPTR
atp_json_build_create_record(STRPTR repo, STRPTR collection,
    STRPTR rkey, STRPTR record_json)
{
    ULONG need;
    STRPTR buf;
    ULONG used;

    if (repo == NULL || collection == NULL || record_json == NULL) {
        return NULL;
    }
    need = 128 + (ULONG)strlen(repo) + (ULONG)strlen(collection) +
        (ULONG)strlen(record_json);
    if (rkey != NULL) {
        need += (ULONG)strlen(rkey) + 16;
    }
    buf = (STRPTR)atp_alloc(need);
    if (buf == NULL) {
        return NULL;
    }
    strcpy((char *)buf, "{\"repo\":");
    used = (ULONG)strlen((char *)buf);
    if (!atp_json_append_escaped(buf, need, &used, repo)) {
        atp_free(buf);
        return NULL;
    }
    strcpy((char *)buf + used, ",\"collection\":");
    used = (ULONG)strlen((char *)buf);
    if (!atp_json_append_escaped(buf, need, &used, collection)) {
        atp_free(buf);
        return NULL;
    }
    if (rkey != NULL && rkey[0] != '\0') {
        strcpy((char *)buf + used, ",\"rkey\":");
        used = (ULONG)strlen((char *)buf);
        if (!atp_json_append_escaped(buf, need, &used, rkey)) {
            atp_free(buf);
            return NULL;
        }
    }
    strcpy((char *)buf + used, ",\"record\":");
    used = (ULONG)strlen((char *)buf);
    if (used + (ULONG)strlen(record_json) + 2 >= need) {
        atp_free(buf);
        return NULL;
    }
    strcpy((char *)buf + used, (char *)record_json);
    used = (ULONG)strlen((char *)buf);
    buf[used++] = '}';
    buf[used] = '\0';
    return buf;
}

STRPTR
atp_json_build_put_record(STRPTR repo, STRPTR collection,
    STRPTR rkey, STRPTR record_json)
{
    /* putRecord requires rkey; same shape as create with rkey. */
    if (rkey == NULL || rkey[0] == '\0') {
        return NULL;
    }
    return atp_json_build_create_record(repo, collection, rkey, record_json);
}

STRPTR
atp_json_dup_value(STRPTR p)
{
    STRPTR start;
    STRPTR end;

    if (p == NULL) {
        return NULL;
    }
    start = atp_json_skip(p);
    end = atp_json_skip_value(start);
    if (end <= start) {
        return NULL;
    }
    return atp_strndup(start, (ULONG)(end - start));
}
