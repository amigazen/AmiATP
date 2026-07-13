/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * amiatpbase.h - amiatp.library base (library-private only)
 */

#ifndef ATP_PRIVATE_AMIATPBASE_H
#define ATP_PRIVATE_AMIATPBASE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef SEGLISTPTR
#define SEGLISTPTR BPTR
#endif

struct AmiAtpBase
{
    struct Library          aab_LibNode;
    SEGLISTPTR              aab_SegList;
    struct ExecBase        *aab_SysBase;

    struct Library         *aab_DOSBase;
    struct Library         *aab_UtilityBase;
    struct Library         *aab_HttpBase;

    ULONG                   aab_BreakMask;
    STRPTR                  aab_DefaultUserAgent;
    STRPTR                  aab_CABundlePath;
    STRPTR                  aab_DefaultService;
    STRPTR                  aab_DefaultAppView;

    LONG                    aab_LastError;
    UBYTE                   aab_ErrorString[256];
};

#define AMIATP_LIBNAME "amiatp.library"

#endif /* ATP_PRIVATE_AMIATPBASE_H */
