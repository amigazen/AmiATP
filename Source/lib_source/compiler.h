/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * compiler.h - cross-compiler attributes for amiatp.library
 */

#ifndef ATP_COMPILER_H
#define ATP_COMPILER_H

#include <exec/types.h>
#include <clib/compiler-specific.h>
#include <proto/exec.h>

#ifndef ATP_INITTABLE_DEFINED
#define ATP_INITTABLE_DEFINED 1
struct InitTable
{
    ULONG it_LibSize;
    APTR *it_FuncTable;
    APTR  it_DataTable;
    APTR  it_InitFunc;
};
#endif

struct MyDataInit
{
    ULONG md_Init[19];
};

#endif /* ATP_COMPILER_H */
