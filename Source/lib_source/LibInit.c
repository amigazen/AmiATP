/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * LibInit.c - ROMTag, DataTab, and dependency open/close for amiatp.library
 *
 * Opens dos.library, utility.library, and amihttp.library.
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
#include "private/atp_internal.h"
#include "compiler.h"

#define ATPLIBNAME "amiatp"
#define ATPLIBVER  " 1.0 (13.7.2026)"

const char ATP_LibName[] = ATPLIBNAME ".library";
const char ATP_LibID[]   = ATPLIBNAME ATPLIBVER;
const char ATP_VerString[] = "\0$VER: " ATPLIBNAME ATPLIBVER;

extern struct ExecBase *SysBase;
extern struct AmiAtpBase *AtpBase;
extern struct Library *DOSBase;
extern struct Library *UtilityBase;
extern struct Library *HttpBase;

ULONG __SAVE_DS__
L_OpenLibs(struct AmiAtpBase *base)
{
    SysBase = *((struct ExecBase **)4);

    if (base != NULL) {
        base->aab_DOSBase = NULL;
        base->aab_UtilityBase = NULL;
        base->aab_HttpBase = NULL;
        base->aab_LastError = 0;
    }

    DOSBase = OpenLibrary((STRPTR)"dos.library", 37);
    if (DOSBase == NULL) {
        return 1;
    }
    if (base != NULL) {
        base->aab_DOSBase = DOSBase;
    }

    UtilityBase = OpenLibrary((STRPTR)"utility.library", 36);
    if (UtilityBase == NULL) {
        CloseLibrary(DOSBase);
        DOSBase = NULL;
        if (base != NULL) {
            base->aab_DOSBase = NULL;
        }
        return 1;
    }
    if (base != NULL) {
        base->aab_UtilityBase = UtilityBase;
    }

    HttpBase = OpenLibrary((STRPTR)AMIHTTPNAME, AMIHTTPVERSION);
    if (HttpBase == NULL) {
        CloseLibrary(UtilityBase);
        CloseLibrary(DOSBase);
        UtilityBase = NULL;
        DOSBase = NULL;
        if (base != NULL) {
            base->aab_UtilityBase = NULL;
            base->aab_DOSBase = NULL;
        }
        return 1;
    }
    if (base != NULL) {
        base->aab_HttpBase = HttpBase;
    }

    return 0;
}

void __SAVE_DS__
L_CloseLibs(void)
{
    if (HttpBase != NULL) {
        CloseLibrary(HttpBase);
        HttpBase = NULL;
    }
    if (UtilityBase != NULL) {
        CloseLibrary(UtilityBase);
        UtilityBase = NULL;
    }
    if (DOSBase != NULL) {
        CloseLibrary(DOSBase);
        DOSBase = NULL;
    }
    if (AtpBase != NULL) {
        atp_free(AtpBase->aab_DefaultUserAgent);
        atp_free(AtpBase->aab_CABundlePath);
        atp_free(AtpBase->aab_DefaultService);
        atp_free(AtpBase->aab_DefaultAppView);
        AtpBase->aab_DefaultUserAgent = NULL;
        AtpBase->aab_CABundlePath = NULL;
        AtpBase->aab_DefaultService = NULL;
        AtpBase->aab_DefaultAppView = NULL;
        AtpBase->aab_HttpBase = NULL;
        AtpBase->aab_UtilityBase = NULL;
        AtpBase->aab_DOSBase = NULL;
    }
}

extern struct InitTable InitTab;
extern APTR EndResident;

struct Resident ROMTag = {
    RTC_MATCHWORD,
    &ROMTag,
    &EndResident,
    RTF_AUTOINIT,
    ATP_LIB_VERSION,
    NT_LIBRARY,
    0,
    (APTR)ATP_LibName,
    (APTR)ATP_LibID,
    (APTR)&InitTab
};

APTR EndResident;

struct MyDataInit DataTab = {
    0xE000, 8,  NT_LIBRARY,
    0x80,   10, (ULONG)ATP_LibName,
    0xE000, 14, LIBF_SUMUSED | LIBF_CHANGED,
    0xE000, 20, ATP_LIB_VERSION,
    0xE000, 22, ATP_LIB_REVISION,
    0x80,   24, (ULONG)ATP_LibID,
    (ULONG)0
};

#ifdef __SASC
void __regargs __chkabort(void) { }
void __regargs _CXBRK(void)     { }
#endif
