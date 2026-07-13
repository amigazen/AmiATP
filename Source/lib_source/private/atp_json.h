/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * atp_json.h - Private AT-scoped JSON helpers (not a public API)
 */

#ifndef ATP_PRIVATE_ATP_JSON_H
#define ATP_PRIVATE_ATP_JSON_H

#include <exec/types.h>

/* Skip whitespace. Returns updated pointer. */
STRPTR atp_json_skip(STRPTR p);

/*
 * Find a top-level object key's value start (after ':').
 * Does not descend into nested objects for the key search at each level;
 * use path helpers for nested fields.
 * Returns pointer to value token or NULL.
 */
STRPTR atp_json_find_key(STRPTR json, STRPTR key);

/* Extract JSON string value at p into buf (unescaped, NUL-terminated). */
LONG atp_json_get_string(STRPTR p, STRPTR buf, ULONG buflen);

/* Convenience: find key then get string. */
LONG atp_json_object_get_string(STRPTR json, STRPTR key,
    STRPTR buf, ULONG buflen);

/* Allocate strdup of string value for key (caller FreeVec). */
STRPTR atp_json_object_dup_string(STRPTR json, STRPTR key);

/* Parse integer (no fractions). */
LONG atp_json_object_get_long(STRPTR json, STRPTR key, LONG *out);

/*
 * Locate the Nth element of a JSON array named key at the top level of json.
 * *elem_start / *elem_end delimit the element text (not including commas).
 */
LONG atp_json_array_nth(STRPTR json, STRPTR array_key,
    ULONG index, STRPTR *elem_start, STRPTR *elem_end);

/* Count elements in top-level array named key. */
ULONG atp_json_array_count(STRPTR json, STRPTR array_key);

/*
 * Within an object fragment [start,end), find key and dup string.
 */
STRPTR atp_json_fragment_dup_string(STRPTR start, STRPTR end,
    STRPTR key);

/* Nested: object.key.subkey string (one level of nesting). */
STRPTR atp_json_object_dup_string2(STRPTR json,
    STRPTR key, STRPTR subkey);

/* Nested three levels: feed[i].post.record.text style via fragment. */
STRPTR atp_json_fragment_dup_string2(STRPTR start, STRPTR end,
    STRPTR key, STRPTR subkey);

/* Escape and append a JSON string value (quotes included) into dest. */
LONG atp_json_append_escaped(STRPTR dest, ULONG destlen, ULONG *used,
    STRPTR s);

/* Build createSession body. Caller FreeVec. */
STRPTR atp_json_build_create_session(STRPTR identifier,
    STRPTR password);

/* Build app.bsky.feed.post record JSON. Caller FreeVec. */
STRPTR atp_json_build_post_record(STRPTR text, STRPTR created_at);

/* Build createRecord request body. Caller FreeVec. */
STRPTR atp_json_build_create_record(STRPTR repo, STRPTR collection,
    STRPTR rkey, STRPTR record_json);

/* Build putRecord request body. Caller FreeVec. */
STRPTR atp_json_build_put_record(STRPTR repo, STRPTR collection,
    STRPTR rkey, STRPTR record_json);

/* Duplicate a JSON value token starting at p (object/array/string/atom). */
STRPTR atp_json_dup_value(STRPTR p);

#endif /* ATP_PRIVATE_ATP_JSON_H */
