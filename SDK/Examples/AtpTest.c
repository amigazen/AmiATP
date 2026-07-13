/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * AtpTest.c - Canonical CLI regression harness for amiatp.library
 *
 *   AtpTest LOGIN HANDLE=<h> PASSWORD=<app-password> [CAFILE=] [VERBOSE]
 *   AtpTest LOGIN ... TIMELINE [LIMIT=n]
 *   AtpTest LOGIN ... POST TEXT="hello"
 *   AtpTest LOGIN ... WHOAMI
 *   AtpTest PROFILE HANDLE=<h> [CAFILE=]
 *   AtpTest RESOLVE HANDLE=<h> [CAFILE=]
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/amiatp.h>
#include <stdio.h>
#include <string.h>

struct Library *AtpBase;

static const char version_tag[] = "\0$VER: AtpTest 1.1 (13.7.2026)";

#define TEMPLATE \
    "LOGIN/S,LOGOUT/S,WHOAMI/S,TIMELINE/S,PROFILE/S,POST/S,RESOLVE/S,VERBOSE/S," \
    "HANDLE/K,PASSWORD/K,TEXT/K,CAFILE/K,SERVICE/K,APPVIEW/K,LIMIT/N"

struct Args
{
    LONG   LOGIN;
    LONG   LOGOUT;
    LONG   WHOAMI;
    LONG   TIMELINE;
    LONG   PROFILE;
    LONG   POST;
    LONG   RESOLVE;
    LONG   VERBOSE;
    STRPTR HANDLE;
    STRPTR PASSWORD;
    STRPTR TEXT;
    STRPTR CAFILE;
    STRPTR SERVICE;
    STRPTR APPVIEW;
    LONG  *LIMIT;
};

static void
print_err(struct AtpConnection *conn, LONG err)
{
    STRPTR s;
    LONG cerr;

    s = AtpGetErrorString(err);
    cerr = 0;
    if (conn != NULL) {
        cerr = AtpConnectionGetLastError(conn);
    }
    Printf("AtpTest: error %ld (%s)", err, s != NULL ? s : (STRPTR)"?");
    if (cerr != 0 && cerr != err) {
        Printf(" conn=%ld", cerr);
    }
    PutStr("\n");
}

static void
print_usage(void)
{
    PutStr(
        "AtpTest - amiatp.library regression harness\n\n"
        "  AtpTest LOGIN HANDLE=<h> PASSWORD=<app-password> [CAFILE=] [VERBOSE]\n"
        "  AtpTest LOGIN ... TIMELINE [LIMIT=n]\n"
        "  AtpTest LOGIN ... POST TEXT=\"hello from Amiga\"\n"
        "  AtpTest LOGIN ... WHOAMI\n"
        "  AtpTest PROFILE HANDLE=<h> [CAFILE=]\n"
        "  AtpTest RESOLVE HANDLE=<h> [CAFILE=]\n"
        "  VERBOSE — rich XRPC debug (URLs, status, body previews)\n");
}

int
main(void)
{
    struct RDArgs *rdargs;
    struct Args args;
    struct AtpConnection *conn;
    LONG err;
    ULONG limit;
    LONG did_cmd;
    UBYTE did[128];

    (void)version_tag;
    memset(&args, 0, sizeof(args));

    AtpBase = OpenLibrary(AMIATPNAME, AMIATPVERSION);
    if (AtpBase == NULL) {
        PutStr("AtpTest: cannot open amiatp.library\n");
        return RETURN_FAIL;
    }

    rdargs = ReadArgs(TEMPLATE, (LONG *)&args, NULL);
    if (rdargs == NULL) {
        print_usage();
        CloseLibrary(AtpBase);
        return RETURN_ERROR;
    }

    limit = 20;
    if (args.LIMIT != NULL && *args.LIMIT > 0) {
        limit = (ULONG)*args.LIMIT;
    }

    conn = NewAtpConnection();
    if (conn == NULL) {
        PutStr("AtpTest: NewAtpConnection failed\n");
        FreeArgs(rdargs);
        CloseLibrary(AtpBase);
        return RETURN_FAIL;
    }

    SetAtpConnectionAttrs(conn,
        ATPA_USERAGENT, (ULONG)"AtpTest/1.1",
        ATPA_CONNECT_TIMEOUT, 15,
        ATPA_READ_TIMEOUT, 30,
        ATPA_VERBOSE, (ULONG)(args.VERBOSE ? TRUE : FALSE),
        TAG_DONE);

    if (args.CAFILE != NULL) {
        SetAtpConnectionAttrs(conn,
            ATPA_CA_BUNDLE, (ULONG)args.CAFILE,
            TAG_DONE);
        if (args.VERBOSE) {
            Printf("AtpTest: CAFILE=%s\n", args.CAFILE);
        }
    }
    if (args.SERVICE != NULL) {
        SetAtpConnectionAttrs(conn,
            ATPA_SERVICE, (ULONG)args.SERVICE,
            TAG_DONE);
    }
    if (args.APPVIEW != NULL) {
        SetAtpConnectionAttrs(conn,
            ATPA_APPVIEW, (ULONG)args.APPVIEW,
            TAG_DONE);
    }

    did_cmd = 0;
    err = 0;

    if (args.LOGIN) {
        did_cmd = 1;
        if (args.HANDLE == NULL || args.PASSWORD == NULL) {
            PutStr("AtpTest: LOGIN needs HANDLE and PASSWORD (app password)\n");
            err = ERROR_ATP_INVALID_ARG;
        } else {
            err = AtpLogin(conn, args.HANDLE, args.PASSWORD);
            if (err != 0) {
                print_err(conn, err);
            } else {
                Printf("AtpTest: logged in as @%s\n  %s\n",
                    AtpConnectionGetHandle(conn),
                    AtpConnectionGetDid(conn));
            }
        }
        if (err != 0) {
            DisposeAtpConnection(conn);
            FreeArgs(rdargs);
            CloseLibrary(AtpBase);
            return RETURN_ERROR;
        }
    }

    if (args.WHOAMI) {
        STRPTR h;
        STRPTR d;

        did_cmd = 1;
        h = AtpConnectionGetHandle(conn);
        d = AtpConnectionGetDid(conn);
        if (h == NULL || d == NULL) {
            PutStr("AtpTest: not logged in\n");
            err = ERROR_ATP_NOT_LOGGED_IN;
        } else {
            Printf("AtpTest: @%s\n  %s\n", h, d);
        }
    }

    if (args.TIMELINE) {
        struct AtpFeed *feed;
        ULONG i;
        ULONG n;
        STRPTR cursor;

        did_cmd = 1;
        feed = NULL;
        err = AtpGetTimeline(conn, limit, NULL, &feed);
        if (err != 0) {
            print_err(conn, err);
        } else {
            n = AtpFeedGetCount(feed);
            Printf("AtpTest: timeline (%lu)\n", (unsigned long)n);
            for (i = 0; i < n; i++) {
                struct AtpFeedPost *post;
                STRPTR author;
                STRPTR text;
                STRPTR uri;

                post = AtpFeedGetPost(feed, i);
                if (post == NULL) {
                    continue;
                }
                author = AtpFeedPostGetAuthorHandle(post);
                text = AtpFeedPostGetText(post);
                uri = AtpFeedPostGetUri(post);
                Printf("----\n@%s\n%s\n",
                    author != NULL ? author : (STRPTR)"?",
                    text != NULL ? text : (STRPTR)"");
                if (args.VERBOSE && uri != NULL) {
                    Printf("  uri=%s\n", uri);
                }
            }
            cursor = AtpFeedGetCursor(feed);
            if (args.VERBOSE && cursor != NULL) {
                Printf("AtpTest: cursor=%s\n", cursor);
            }
            DisposeAtpFeed(feed);
        }
    }

    if (args.PROFILE) {
        struct AtpProfile *prof;
        STRPTR actor;

        did_cmd = 1;
        actor = args.HANDLE;
        if (actor == NULL) {
            actor = AtpConnectionGetHandle(conn);
        }
        if (actor == NULL) {
            PutStr("AtpTest: PROFILE needs HANDLE or a login session\n");
            err = ERROR_ATP_INVALID_ARG;
        } else {
            prof = NULL;
            err = AtpGetProfile(conn, actor, &prof);
            if (err != 0) {
                print_err(conn, err);
            } else {
                Printf("AtpTest: profile @%s\n",
                    AtpProfileGetHandle(prof) != NULL ?
                        AtpProfileGetHandle(prof) : actor);
                if (AtpProfileGetDisplayName(prof) != NULL) {
                    Printf("  %s\n", AtpProfileGetDisplayName(prof));
                }
                if (AtpProfileGetDescription(prof) != NULL) {
                    Printf("  %s\n", AtpProfileGetDescription(prof));
                }
                DisposeAtpProfile(prof);
            }
        }
    }

    if (args.POST) {
        UBYTE uri[256];

        did_cmd = 1;
        if (args.TEXT == NULL || args.TEXT[0] == '\0') {
            PutStr("AtpTest: POST requires TEXT=\"...\"\n");
            err = ERROR_ATP_INVALID_ARG;
        } else {
            uri[0] = '\0';
            err = AtpCreatePost(conn, args.TEXT, (STRPTR)uri, sizeof(uri));
            if (err != 0) {
                print_err(conn, err);
            } else if (uri[0] == '\0') {
                PutStr("AtpTest: POST returned success but empty at:// URI\n");
                err = ERROR_ATP_XRPC;
            } else {
                Printf("AtpTest: posted %s\n", uri);
            }
        }
    }

    if (args.RESOLVE) {
        did_cmd = 1;
        if (args.HANDLE == NULL) {
            PutStr("AtpTest: RESOLVE requires HANDLE\n");
            err = ERROR_ATP_INVALID_ARG;
        } else {
            err = AtpResolveHandle(conn, args.HANDLE, (STRPTR)did, sizeof(did));
            if (err != 0) {
                print_err(conn, err);
            } else {
                Printf("AtpTest: %s -> %s\n", args.HANDLE, did);
            }
        }
    }

    if (args.LOGOUT) {
        did_cmd = 1;
        AtpLogout(conn);
        PutStr("AtpTest: logged out\n");
    }

    if (!did_cmd) {
        print_usage();
        DisposeAtpConnection(conn);
        FreeArgs(rdargs);
        CloseLibrary(AtpBase);
        return RETURN_ERROR;
    }

    if (args.LOGIN && !args.LOGOUT) {
        AtpLogout(conn);
    }

    DisposeAtpConnection(conn);
    FreeArgs(rdargs);
    CloseLibrary(AtpBase);
    return (err == 0) ? RETURN_OK : RETURN_ERROR;
}
