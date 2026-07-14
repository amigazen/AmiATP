/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * StartUp.c - LVO trap, function vector table, and library lifecycle
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/initializers.h>

#include <proto/exec.h>

#include "private/amiatpbase.h"
#include "private/atp_build.h"
#include "compiler.h"
#include "amiatp_funcs.h"

extern ULONG L_OpenLibs(struct AmiAtpBase *base);
extern void L_CloseLibs(void);

extern struct Resident ROMTag;
extern const char ATP_LibName[];
extern const char ATP_LibID[];
extern struct MyDataInit DataTab;

struct AmiAtpBase *AtpBase = NULL;
struct ExecBase *SysBase = NULL;
struct Library *DOSBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *HttpBase = NULL;

LONG __ASM__ LibStart(void);
struct AmiAtpBase * __ASM__ __SAVE_DS__ InitLib(
    __REG__(a6, struct ExecBase *sysbase),
    __REG__(a0, BPTR seglist),
    __REG__(d0, struct AmiAtpBase *base));
struct AmiAtpBase * __ASM__ __SAVE_DS__ OpenLib(
    __REG__(a6, struct AmiAtpBase *base));
BPTR __ASM__ __SAVE_DS__ CloseLib(
    __REG__(a6, struct AmiAtpBase *base));
BPTR __ASM__ __SAVE_DS__ ExpungeLib(
    __REG__(a6, struct AmiAtpBase *base));
ULONG __ASM__ ExtFuncLib(void);

APTR FuncTab[];

/*
 * FuncTab[] order MUST match SDK/SFD/amiatp_lib.sfd and pragmas
 * (bias 0x1e, +6 per LVO).
 */

struct InitTable InitTab = {
    (ULONG)sizeof(struct AmiAtpBase),
    (APTR *)FuncTab,
    (APTR)&DataTab,
    (APTR)InitLib
};

APTR FuncTab[] = {
    (APTR)OpenLib,
    (APTR)CloseLib,
    (APTR)ExpungeLib,
    (APTR)ExtFuncLib,
    (APTR)AtpBaseTagsA,
    (APTR)AtpError,
    (APTR)AtpGetErrorString,
    (APTR)NewAtpConnection,
    (APTR)DisposeAtpConnection,
    (APTR)SetAtpConnectionAttrsA,
    (APTR)AtpLogin,
    (APTR)AtpRefreshSession,
    (APTR)AtpLogout,
    (APTR)AtpConnectionGetDid,
    (APTR)AtpConnectionGetHandle,
    (APTR)AtpConnectionGetLastError,
    (APTR)NewAtpRecord,
    (APTR)DisposeAtpRecord,
    (APTR)SetAtpRecordAttrsA,
    (APTR)AtpGetRecord,
    (APTR)AtpCreateRecord,
    (APTR)AtpPutRecord,
    (APTR)AtpDeleteRecord,
    (APTR)AtpListRecords,
    (APTR)DisposeAtpRecordList,
    (APTR)AtpRecordListGetCount,
    (APTR)AtpRecordListGetRecord,
    (APTR)AtpRecordGetUri,
    (APTR)AtpRecordGetCid,
    (APTR)AtpRecordGetString,
    (APTR)AtpGetTimeline,
    (APTR)AtpGetAuthorFeed,
    (APTR)DisposeAtpFeed,
    (APTR)AtpFeedGetCount,
    (APTR)AtpFeedGetPost,
    (APTR)AtpFeedGetCursor,
    (APTR)AtpFeedPostGetAuthorHandle,
    (APTR)AtpFeedPostGetText,
    (APTR)AtpFeedPostGetUri,
    (APTR)AtpGetProfile,
    (APTR)DisposeAtpProfile,
    (APTR)AtpProfileGetHandle,
    (APTR)AtpProfileGetDisplayName,
    (APTR)AtpProfileGetDescription,
    (APTR)AtpCreatePost,
    (APTR)AtpResolveHandle,
    (APTR)AtpFeedPostGetAvatarUrl,
    (APTR)AtpFeedPostGetImageCount,
    (APTR)AtpFeedPostGetImageUrl,
    (APTR)AtpDownloadUrl,
    (APTR)((LONG)-1)
};

LONG
__ASM__ LibStart(void)
{
    return -1;
}

struct AmiAtpBase *
__ASM__ __SAVE_DS__ InitLib(
    __REG__(a6, struct ExecBase *sysbase),
    __REG__(a0, BPTR seglist),
    __REG__(d0, struct AmiAtpBase *base))
{
    AtpBase = base;
    SysBase = sysbase;

    base->aab_LibNode.lib_Node.ln_Type = NT_LIBRARY;
    base->aab_LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    base->aab_LibNode.lib_Version = ATP_LIB_VERSION;
    base->aab_LibNode.lib_Revision = ATP_LIB_REVISION;
    base->aab_LibNode.lib_IdString = (STRPTR)ATP_LibID;

    base->aab_SegList = seglist;
    base->aab_SysBase = sysbase;

    if (L_OpenLibs(base) != 0) {
        return (struct AmiAtpBase *)NULL;
    }

    return base;
}

struct AmiAtpBase *
__ASM__ __SAVE_DS__ OpenLib(__REG__(a6, struct AmiAtpBase *base))
{
    base->aab_LibNode.lib_OpenCnt++;
    base->aab_LibNode.lib_Flags &= ~LIBF_DELEXP;
    return base;
}

BPTR
__ASM__ __SAVE_DS__ CloseLib(__REG__(a6, struct AmiAtpBase *base))
{
    base->aab_LibNode.lib_OpenCnt--;

    if (base->aab_LibNode.lib_OpenCnt == 0) {
        if (base->aab_LibNode.lib_Flags & LIBF_DELEXP) {
            return ExpungeLib(base);
        }
    }

    return 0;
}

BPTR
__ASM__ __SAVE_DS__ ExpungeLib(__REG__(a6, struct AmiAtpBase *base))
{
    BPTR seg;

    if (base->aab_LibNode.lib_OpenCnt != 0) {
        base->aab_LibNode.lib_Flags |= LIBF_DELEXP;
        return 0;
    }

    seg = base->aab_SegList;

    L_CloseLibs();

    Remove(&base->aab_LibNode.lib_Node);
    FreeMem((APTR)((BYTE *)base - base->aab_LibNode.lib_NegSize),
        base->aab_LibNode.lib_NegSize + base->aab_LibNode.lib_PosSize);

    AtpBase = NULL;

    return seg;
}

ULONG
__ASM__ ExtFuncLib(void)
{
    return 0;
}
