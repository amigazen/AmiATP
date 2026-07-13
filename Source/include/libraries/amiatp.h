/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * amiatp.h - Public constants, tags, and opaque types for amiatp.library
 *
 * Amiga-native AT Protocol (XRPC) client.  Transport is amihttp.library
 * (opened internally).  JSON parse/build is private to this library.
 */

#ifndef LIBRARIES_AMIATP_H
#define LIBRARIES_AMIATP_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/****************************************************************************/
/* Library identity                                                         */
/****************************************************************************/

#define AMIATPNAME      "amiatp.library"
#define AMIATPVERSION   1

/****************************************************************************/
/* Opaque handles                                                           */
/****************************************************************************/

struct AtpConnection;
struct AtpRecord;
struct AtpRecordList;
struct AtpFeed;
struct AtpFeedPost;
struct AtpProfile;

/****************************************************************************/
/* Error model                                                              */
/*   AtpError() / AtpConnectionGetLastError() - ERROR_ATP_* transport/API.  */
/*   HTTP wire status from XRPC is mapped into these codes when needed.     */
/****************************************************************************/

#define ERROR_ATP_NOT_IMPLEMENTED       8900
#define ERROR_ATP_OUT_OF_MEMORY         8901
#define ERROR_ATP_INVALID_HANDLE        8902
#define ERROR_ATP_INVALID_ARG           8903
#define ERROR_ATP_NO_HTTP               8904
#define ERROR_ATP_HTTP                  8905
#define ERROR_ATP_HTTP_STATUS           8906
#define ERROR_ATP_JSON                  8907
#define ERROR_ATP_AUTH                  8908
#define ERROR_ATP_NOT_LOGGED_IN         8909
#define ERROR_ATP_BUFFER_TOO_SMALL      8910
#define ERROR_ATP_NOT_FOUND             8911
#define ERROR_ATP_XRPC                  8912

/****************************************************************************/
/* AtpBaseTagList tags (Tier 0)                                             */
/****************************************************************************/

#define ATBT_BREAKMASK              (TAG_USER + 0x01)
#define ATBT_DEFAULT_USERAGENT      (TAG_USER + 0x02)
#define ATBT_CA_BUNDLE_PATH         (TAG_USER + 0x03)
#define ATBT_DEFAULT_SERVICE        (TAG_USER + 0x04)
#define ATBT_DEFAULT_APPVIEW        (TAG_USER + 0x05)

/****************************************************************************/
/* SetAtpConnectionAttrsA tags (Tier 1)                                     */
/****************************************************************************/

#define ATPA_SERVICE                (TAG_USER + 0x100)  /* PDS base URL */
#define ATPA_APPVIEW                (TAG_USER + 0x101)  /* AppView base URL */
#define ATPA_CA_BUNDLE              (TAG_USER + 0x102)
#define ATPA_USERAGENT              (TAG_USER + 0x103)
#define ATPA_CONNECT_TIMEOUT        (TAG_USER + 0x104)
#define ATPA_READ_TIMEOUT           (TAG_USER + 0x105)
/* When TRUE, amiatp Printf()s XRPC method/URL/status/body previews (CLI debug). */
#define ATPA_VERBOSE                (TAG_USER + 0x106)

/****************************************************************************/
/* SetAtpRecordAttrsA tags (Tier 2)                                         */
/****************************************************************************/

#define ATPR_COLLECTION             (TAG_USER + 0x200)
#define ATPR_RKEY                   (TAG_USER + 0x201)
#define ATPR_REPO                   (TAG_USER + 0x202)  /* DID; default session DID */
#define ATPR_JSON_BODY              (TAG_USER + 0x203)  /* escape hatch: full record JSON */
#define ATPR_TEXT                   (TAG_USER + 0x204)  /* convenience for post text */

#endif /* LIBRARIES_AMIATP_H */
