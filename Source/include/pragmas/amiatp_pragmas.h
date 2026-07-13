#ifndef PRAGMAS_AMIATP_PRAGMAS_H
#define PRAGMAS_AMIATP_PRAGMAS_H

/*
** SAS/C / Lattice #pragma libcall entries for amiatp.library
** Offsets match SDK/SFD/amiatp_lib.sfd (bias 0x1E).
*/

#ifndef CLIB_AMIATP_PROTOS_H
#include <clib/amiatp_protos.h>
#endif

#pragma libcall AtpBase AtpBaseTagList 1E 801
#pragma libcall AtpBase AtpError 24 00
#pragma libcall AtpBase AtpGetErrorString 2A 001
#pragma libcall AtpBase NewAtpConnection 30 00
#pragma libcall AtpBase DisposeAtpConnection 36 801
#pragma libcall AtpBase SetAtpConnectionAttrsA 3C 9802
#pragma libcall AtpBase AtpLogin 42 A9803
#pragma libcall AtpBase AtpRefreshSession 48 801
#pragma libcall AtpBase AtpLogout 4E 801
#pragma libcall AtpBase AtpConnectionGetDid 54 801
#pragma libcall AtpBase AtpConnectionGetHandle 5A 801
#pragma libcall AtpBase AtpConnectionGetLastError 60 801
#pragma libcall AtpBase NewAtpRecord 66 00
#pragma libcall AtpBase DisposeAtpRecord 6C 801
#pragma libcall AtpBase SetAtpRecordAttrsA 72 9802
#pragma libcall AtpBase AtpGetRecord 78 9802
#pragma libcall AtpBase AtpCreateRecord 7E 9802
#pragma libcall AtpBase AtpPutRecord 84 9802
#pragma libcall AtpBase AtpDeleteRecord 8A 9802
#pragma libcall AtpBase AtpListRecords 90 BA09805
#pragma libcall AtpBase DisposeAtpRecordList 96 801
#pragma libcall AtpBase AtpRecordListGetCount 9C 801
#pragma libcall AtpBase AtpRecordListGetRecord A2 0802
#pragma libcall AtpBase AtpRecordGetUri A8 801
#pragma libcall AtpBase AtpRecordGetCid AE 801
#pragma libcall AtpBase AtpRecordGetString B4 9802
#pragma libcall AtpBase AtpGetTimeline BA A90804
#pragma libcall AtpBase AtpGetAuthorFeed C0 BA09805
#pragma libcall AtpBase DisposeAtpFeed C6 801
#pragma libcall AtpBase AtpFeedGetCount CC 801
#pragma libcall AtpBase AtpFeedGetPost D2 0802
#pragma libcall AtpBase AtpFeedGetCursor D8 801
#pragma libcall AtpBase AtpFeedPostGetAuthorHandle DE 801
#pragma libcall AtpBase AtpFeedPostGetText E4 801
#pragma libcall AtpBase AtpFeedPostGetUri EA 801
#pragma libcall AtpBase AtpGetProfile F0 A9803
#pragma libcall AtpBase DisposeAtpProfile F6 801
#pragma libcall AtpBase AtpProfileGetHandle FC 801
#pragma libcall AtpBase AtpProfileGetDisplayName 102 801
#pragma libcall AtpBase AtpProfileGetDescription 108 801
#pragma libcall AtpBase AtpCreatePost 10E 0A9804
#pragma libcall AtpBase AtpResolveHandle 114 0A9804
#pragma libcall AtpBase AtpFeedPostGetAvatarUrl 11A 801
#pragma libcall AtpBase AtpFeedPostGetImageCount 120 801
#pragma libcall AtpBase AtpFeedPostGetImageUrl 126 0802
#pragma libcall AtpBase AtpDownloadUrl 12C A9803

#endif /* PRAGMAS_AMIATP_PRAGMAS_H */
