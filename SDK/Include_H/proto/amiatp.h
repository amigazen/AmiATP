#ifndef PROTO_AMIATP_H
#define PROTO_AMIATP_H

#include <clib/amiatp_protos.h>

#ifndef _NO_INLINE

#if defined(LATTICE) || defined(__SASC) || defined(_DCC)

#ifndef PRAGMAS_AMIATP_PRAGMAS_H
#include <pragmas/amiatp_pragmas.h>
#endif

#elif defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)

#ifndef PRAGMA_AMIATP_LIB_H
#include <pragma/amiatp_lib.h>
#endif

#elif defined(__VBCC__)

#ifndef INLINE_AMIATP_PROTOS_H
#include <inline/amiatp_protos.h>
#endif

#elif defined(__GNUC__)

#if defined(mc68000)
# ifndef INLINE_AMIATP_H
#  include <inline/amiatp.h>
# endif
#endif

#endif

#endif /* _NO_INLINE */

#ifndef __NOLIBBASE__
  extern struct Library *
# ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
# endif
  AtpBase;
#endif

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef LIBRARIES_AMIATP_H
#include <libraries/amiatp.h>
#endif

#ifndef ATP_VARARGS_DEFINED
#define ATP_VARARGS_DEFINED 1

static LONG AtpBaseTags(Tag tag1, ...)
{
    return AtpBaseTagList((struct TagItem *)&tag1);
}

static LONG SetAtpConnectionAttrs(struct AtpConnection *conn, Tag tag1, ...)
{
    return SetAtpConnectionAttrsA(conn, (struct TagItem *)&tag1);
}

static LONG SetAtpRecordAttrs(struct AtpRecord *rec, Tag tag1, ...)
{
    return SetAtpRecordAttrsA(rec, (struct TagItem *)&tag1);
}

#endif /* ATP_VARARGS_DEFINED */

#endif /* PROTO_AMIATP_H */
