# amiatp.library

This is **AmiATP**, an AT Protocol (XRPC) client shared library for Amiga.
It provides an Amiga-flavoured C API for applications that need to talk to Bluesky
and other AT Protocol services — session login, repo CRUD, and Lexicon helpers —
without embedding HTTP, TLS, or JSON parsing in every client.

AmiATP in turn uses [AmiHTTP](https://github.com/amigazen/amihttp/) for HTTP(S) transport.

## [amigazen project](http://www.amigazen.com)

*A web, suddenly*

*Forty years meditation*

*Minds awaken, free*

**amigazen project** is using modern software development tools and methods to update and rerelease classic Amiga open source software. Projects include a new AWeb, a new Amiga Python 2, and the ToolKit project - a universal SDK for Amiga.

Key to the amigazen project approach is ensuring every project can be built with the same common set of development tools and configurations, so the ToolKit project was created to provide a standard configuration for Amiga development. All *amigazen project* releases will be guaranteed to build against the ToolKit standard so that anyone can download and begin contributing straightaway without having to tailor the toolchain for their own setup.

AmiATP is an original work of the amigazen project. This software is redistributed on terms described in the documentation, particularly the file LICENSE.md

amigazen project philosophy is based on openness:

*Open* to anyone and everyone	- *Open* source and free for all	- *Open* your mind and create!

PRs for all projects are gratefully received at [GitHub](https://github.com/amigazen/). While the focus now is on classic 68k software, it is intended that all amigazen project releases can be ported to other Amiga-like systems including AROS and MorphOS where feasible.

## About amiatp.library

amiatp.library is a **standalone AT Protocol client** for the Amiga platform.
It does not implement HTTP itself, it opens `amihttp.library` (and thus also uses TLS for HTTPS via either AmiSSL or AmiTLS).

The first public consumer is **Seiten**, the Bluesky client for Amiga - from amigazen project.

## Why a separate library?

| Layer | Library / app |
|-------|----------------|
| TLS | amitls / AmiSSL (via amihttp) |
| HTTP/1.1 | amihttp.library |
| AT Proto / Bluesky XRPC | **amiatp.library** |
| App UI / prefs | Seiten (or your application) |

### Rationale in brief

| Goal | How amiatp addresses it |
|------|-------------------------|
| **Reuse** | One implementation of login JWTs, XRPC, and Lexicon helpers for GUI and CLI |
| **Separation of concerns** | Transport in amihttp; AT Proto in amiatp; chrome and prefs in the app |
| **Stability** | Public SFD and versioned LVOs; internal JSON/HTTP refactors do not break callers |
| **Testability** | `AtpTest` in `SDK/Examples` exercises the API without launching Seiten |
| **Amiga flavour** | Opaque objects, `New*` / `Dispose*`, `Set*AttrsA` + TagItems, typed `ERROR_ATP_*` |

## API tiers

Public LVOs follow Amiga object-library conventions: type names in constructors
(`NewAtpConnection`), `Set*Attrs` for TagItem configuration, and type-qualified
operation names (`AtpGetTimeline`) to avoid flat-namespace clashes.

| Tier | Objects | Use case |
|------|---------|----------|
| 0 | `AtpBaseTagsA` / `AtpBaseTags`, `AtpError`, `AtpGetErrorString` | Per-process defaults (CA path, service / AppView URLs, break mask) |
| 1 | `AtpConnection` | PDS + AppView settings, session JWTs, owns an internal `HttpSession`; login / refresh / logout |
| 2 | `AtpRecord` / `AtpRecordList` | Generic `com.atproto.repo.*` get / create / put / delete / list |
| 3 | Feed / Profile helpers | `app.bsky.*` timeline, author feed, profile, createPost, resolveHandle, CDN download, media getters |

Authenticated `app.bsky.*` calls go to the user’s **PDS**, not the public AppView.
Unauthenticated resolve / public profile paths may use AppView as configured.

### Typical Tier 1 + Tier 3 flow

```c
AtpBase = OpenLibrary(AMIATPNAME, AMIATPVERSION);
conn = NewAtpConnection();
SetAtpConnectionAttrs(conn,
    ATPA_CA_BUNDLE, (ULONG)"PROGDIR:cacert.pem",
    ATPA_USERAGENT, (ULONG)"MyApp/1.0",
    TAG_DONE);
if (AtpLogin(conn, "you.bsky.social", app_password) == 0) {
    struct AtpFeed *feed = NULL;
    if (AtpGetTimeline(conn, 20, NULL, &feed) == 0) {
        /* AtpFeedGetPost / AtpFeedPostGetText ... */
        DisposeAtpFeed(feed);
    }
    AtpLogout(conn);
}
DisposeAtpConnection(conn);
CloseLibrary(AtpBase);
```

Use a Bluesky **app password**, not the account password.

See `SDK/Autodocs/amiatp.doc` and `SDK/Include_H/` for the full LVO set and tags.

## Protocol and feature support

| Feature | Support | Notes |
|---------|---------|-------|
| `com.atproto.server.createSession` / `deleteSession` / refresh | ✅ Full | Access + refresh JWTs held on `AtpConnection` |
| `com.atproto.repo.getRecord` / `createRecord` / `putRecord` / `deleteRecord` / `listRecords` | ✅ Full | Via `AtpRecord` / `AtpRecordList` |
| `app.bsky.feed.getTimeline` / `getAuthorFeed` | ✅ Full | Cursor paging; posts + avatar / embed thumb URLs |
| `app.bsky.actor.getProfile` | ✅ Full | Handle, display name, description |
| `app.bsky.feed.post` create | ✅ Full | Text posts via `AtpCreatePost` / record helpers |
| `com.atproto.identity.resolveHandle` | ✅ Full | Handle → DID |
| CDN / URL download | ✅ Full | `AtpDownloadUrl` (GET → DOS file; size-capped) |
| Lexicon reply / like / repost records | ❌ Partial | Planned; apps may stub UI until wired |
| Public JSON API | ❌ | Private Lexicon JSON only |
| blob upload / media posting | ❌ | Later |

## Build

On Amiga with SAS/C against the ToolKit standard:

```
cd Source/lib_source
smake
smake install
smake headers
```

### Prerequisites / dependencies

Building AmiATP requires:

- SAS/C
- NDK 3.2 (system headers on `INCLUDE:`)
- [amihttp.library](https://github.com/amigazen/amihttp/)
- [amitls.library](https://github.com/amigazen/amitls/)
- Roadshow / bsdsocket headers as required by amihttp in netinclude:

## Examples

`SDK/Examples/AtpTest` is the canonical CLI regression harness:

```
cd SDK/Examples
smake
```

```
AtpTest LOGIN HANDLE=yourusername.bsky.social PASSWORD=xxxx-xxxx-xxxx-xxxx CAFILE=PROGDIR:cacert.pem TIMELINE VERBOSE
AtpTest LOGIN ... POST TEXT="hello from Amiga" VERBOSE
AtpTest LOGIN ... WHOAMI
AtpTest PROFILE HANDLE=yourusername.bsky.social CAFILE=PROGDIR:cacert.pem
AtpTest RESOLVE HANDLE=yourusername.bsky.social CAFILE=PROGDIR:cacert.pem
```


## Contact

- At GitHub https://github.com/amigazen/amiatp/
- on the web at http://www.amigazen.com/ (Amiga browser compatible)
- or email aweb@amigazen.com

## Acknowledgements

*Amiga* is a trademark of **Amiga Inc**.