/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * amiatp_funcs.h - LVO function declarations for FuncTab
 */

#ifndef AMIATP_FUNCS_H
#define AMIATP_FUNCS_H

#include <exec/types.h>
#include <libraries/amiatp.h>
#include "compiler.h"

struct AmiAtpBase;

LONG __ASM__ __SAVE_DS__ AtpBaseTagsA(__REG__(a0, struct TagItem *tags),
    __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpError(__REG__(a6, struct AmiAtpBase *libbase));
STRPTR __ASM__ __SAVE_DS__ AtpGetErrorString(__REG__(d0, LONG code));

struct AtpConnection * __ASM__ __SAVE_DS__ NewAtpConnection(
    __REG__(a6, struct AmiAtpBase *libbase));
VOID __ASM__ __SAVE_DS__ DisposeAtpConnection(
    __REG__(a0, struct AtpConnection *conn));
LONG __ASM__ __SAVE_DS__ SetAtpConnectionAttrsA(
    __REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct TagItem *tags),
    __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpLogin(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR identifier), __REG__(a2, STRPTR password),
    __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpRefreshSession(
    __REG__(a0, struct AtpConnection *conn),
    __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpLogout(__REG__(a0, struct AtpConnection *conn),
    __REG__(a6, struct AmiAtpBase *libbase));
STRPTR __ASM__ __SAVE_DS__ AtpConnectionGetDid(
    __REG__(a0, struct AtpConnection *conn));
STRPTR __ASM__ __SAVE_DS__ AtpConnectionGetHandle(
    __REG__(a0, struct AtpConnection *conn));
LONG __ASM__ __SAVE_DS__ AtpConnectionGetLastError(
    __REG__(a0, struct AtpConnection *conn));

struct AtpRecord * __ASM__ __SAVE_DS__ NewAtpRecord(
    __REG__(a6, struct AmiAtpBase *libbase));
VOID __ASM__ __SAVE_DS__ DisposeAtpRecord(__REG__(a0, struct AtpRecord *rec));
LONG __ASM__ __SAVE_DS__ SetAtpRecordAttrsA(__REG__(a0, struct AtpRecord *rec),
    __REG__(a1, struct TagItem *tags), __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpGetRecord(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct AtpRecord *rec), __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpCreateRecord(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct AtpRecord *rec), __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpPutRecord(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct AtpRecord *rec), __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpDeleteRecord(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, struct AtpRecord *rec), __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpListRecords(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR collection), __REG__(d0, ULONG limit),
    __REG__(a2, STRPTR cursor), __REG__(a3, struct AtpRecordList **out_list),
    __REG__(a6, struct AmiAtpBase *libbase));
VOID __ASM__ __SAVE_DS__ DisposeAtpRecordList(
    __REG__(a0, struct AtpRecordList *list));
ULONG __ASM__ __SAVE_DS__ AtpRecordListGetCount(
    __REG__(a0, struct AtpRecordList *list));
struct AtpRecord * __ASM__ __SAVE_DS__ AtpRecordListGetRecord(
    __REG__(a0, struct AtpRecordList *list), __REG__(d0, ULONG index));
STRPTR __ASM__ __SAVE_DS__ AtpRecordGetUri(__REG__(a0, struct AtpRecord *rec));
STRPTR __ASM__ __SAVE_DS__ AtpRecordGetCid(__REG__(a0, struct AtpRecord *rec));
STRPTR __ASM__ __SAVE_DS__ AtpRecordGetString(__REG__(a0, struct AtpRecord *rec),
    __REG__(a1, STRPTR key));

LONG __ASM__ __SAVE_DS__ AtpGetTimeline(__REG__(a0, struct AtpConnection *conn),
    __REG__(d0, ULONG limit), __REG__(a1, STRPTR cursor),
    __REG__(a2, struct AtpFeed **out_feed),
    __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpGetAuthorFeed(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR actor), __REG__(d0, ULONG limit),
    __REG__(a2, STRPTR cursor), __REG__(a3, struct AtpFeed **out_feed),
    __REG__(a6, struct AmiAtpBase *libbase));
VOID __ASM__ __SAVE_DS__ DisposeAtpFeed(__REG__(a0, struct AtpFeed *feed));
ULONG __ASM__ __SAVE_DS__ AtpFeedGetCount(__REG__(a0, struct AtpFeed *feed));
struct AtpFeedPost * __ASM__ __SAVE_DS__ AtpFeedGetPost(
    __REG__(a0, struct AtpFeed *feed), __REG__(d0, ULONG index));
STRPTR __ASM__ __SAVE_DS__ AtpFeedGetCursor(__REG__(a0, struct AtpFeed *feed));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetAuthorHandle(
    __REG__(a0, struct AtpFeedPost *post));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetText(
    __REG__(a0, struct AtpFeedPost *post));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetUri(
    __REG__(a0, struct AtpFeedPost *post));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetAvatarUrl(
    __REG__(a0, struct AtpFeedPost *post));
ULONG __ASM__ __SAVE_DS__ AtpFeedPostGetImageCount(
    __REG__(a0, struct AtpFeedPost *post));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetImageUrl(
    __REG__(a0, struct AtpFeedPost *post), __REG__(d0, ULONG index));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetExtUri(
    __REG__(a0, struct AtpFeedPost *post));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetExtTitle(
    __REG__(a0, struct AtpFeedPost *post));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetExtDescription(
    __REG__(a0, struct AtpFeedPost *post));
STRPTR __ASM__ __SAVE_DS__ AtpFeedPostGetExtThumb(
    __REG__(a0, struct AtpFeedPost *post));
LONG __ASM__ __SAVE_DS__ AtpGetProfile(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR actor), __REG__(a2, struct AtpProfile **out_profile),
    __REG__(a6, struct AmiAtpBase *libbase));
VOID __ASM__ __SAVE_DS__ DisposeAtpProfile(
    __REG__(a0, struct AtpProfile *profile));
STRPTR __ASM__ __SAVE_DS__ AtpProfileGetHandle(
    __REG__(a0, struct AtpProfile *profile));
STRPTR __ASM__ __SAVE_DS__ AtpProfileGetDisplayName(
    __REG__(a0, struct AtpProfile *profile));
STRPTR __ASM__ __SAVE_DS__ AtpProfileGetDescription(
    __REG__(a0, struct AtpProfile *profile));
LONG __ASM__ __SAVE_DS__ AtpCreatePost(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR text), __REG__(a2, STRPTR uri_buf),
    __REG__(d0, ULONG uri_buflen), __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpResolveHandle(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR handle), __REG__(a2, STRPTR did_buf),
    __REG__(d0, ULONG did_buflen), __REG__(a6, struct AmiAtpBase *libbase));
LONG __ASM__ __SAVE_DS__ AtpDownloadUrl(__REG__(a0, struct AtpConnection *conn),
    __REG__(a1, STRPTR url), __REG__(a2, STRPTR path),
    __REG__(a6, struct AmiAtpBase *libbase));

#endif /* AMIATP_FUNCS_H */
