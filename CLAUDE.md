# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

The **grid-admin** plugin for SmartMet Server. It provides an HTTP interface to the Content Information Storage, allowing external systems (e.g. feeding systems) to query and update content information. It also serves a simple HTML browser UI for inspecting server state (engines, plugins, installed software).

The plugin connects directly to the Content Server using its own configuration — it does **not** use the Grid Engine for content server access, though it does use the Grid Engine for browser UI features.

## Build commands

```bash
make                # Build grid-admin.so
make clean          # Clean build artifacts (removes doc/html/)
make install        # Install to $(plugindir)
make rpm            # Build RPM package
make test           # Run tests (currently no test suite exists)
make configtest     # Validate config with cfgvalidate (if installed)
make format         # No-op — clang-format is disabled (.clang-format sets DisableFormat: true)
make doc            # Generate Doxygen HTML documentation in doc/html/
```

CORBA support is enabled by default. To disable: `make CORBA=disabled`.

## Source layout

All C++ source and headers live in the `grid-admin/` subdirectory. The Makefile compiles everything under that path into `grid-admin.so` (output at repo root). There is no separate `src/` or `include/` split — `.h` and `.cpp` files are co-located.

## Dependencies

- **Libraries**: `smartmet-library-grid-files`, `smartmet-library-grid-content`, `smartmet-library-spine`, `smartmet-library-macgyver`
- **Engine**: `smartmet-engine-grid` (resolved at runtime by the server)
- **System**: `gdal`, `libconfig` (configpp), `omniORB4` (when CORBA enabled), `openssl` (SHA1 for auth)
- **pkg-config**: `REQUIRES = gdal configpp`

## Architecture

The plugin has only two source files:

- **Plugin.cpp/.h** — Standard SmartMet plugin lifecycle (`init`, `shutdown`, `requestHandler`). Registers as a **private** content handler at `/grid-admin` (i.e. `isAdminQuery()` returns `true`). Routes requests: if a `method` parameter is present, delegates to the Content Server HTTP API (`apiRequest`); otherwise delegates to the `Browser` for HTML UI pages. Supports four content server backends: Redis (default), PostgreSQL, HTTP, and CORBA.
- **Browser.cpp/.h** — Generates HTML pages for browsing server state (start, engines, plugins, software). Handles session management (cookies), authentication (optional, SHA1 challenge-response), and delegates grid-engine browsing to `Engine::Grid::Engine::browserRequest()`.

### Dual message processors

`Plugin` holds two `ContentServer::HTTP::ServerInterface` instances:
- `itsMessageProcessor1` — wraps `itsContentServer` (direct Redis/PostgreSQL/CORBA/HTTP backend)
- `itsMessageProcessor2` — wraps the Grid Engine's cached content server (`itsGridEngine->getContentServer_sptr()`)

API requests go to `itsMessageProcessor1` by default. Adding `source=engine` routes to `itsMessageProcessor2` instead (reads from the engine's in-memory cache rather than the backing store).

### Request routing

1. `GET /grid-admin?method=...` — Content Server HTTP API call. The `source=engine` parameter routes through the Grid Engine's cached content server; otherwise routes to the direct content server (Redis/PostgreSQL/CORBA/HTTP).
2. `GET /grid-admin?page=...&target=...` — Browser UI. Pages: `start`, `engines`, `plugins`, `software`, `login`, `logout`. When `target=grid-engine`, delegates to the grid engine's browser.

### Authentication and sessions

Sessions are in-process (not shared across server restarts), expire after 1 hour, and are tracked by a `sessionId` cookie. When `authenticationRequired = true`:

- The login page generates a per-session random key, embeds a JavaScript SHA1 implementation, and sends `username:SHA1(password+password+password+key)` via XHR POST.
- Server-side `Browser::countHash()` performs the same SHA1 keyed hash to verify.
- Successful login stores `UserInfo` in the session; `getUserId() == 0` means unauthenticated.
- The `grid-content-view` permission controls Grid Engine browser access per user.

Users are loaded from CSV files specified by `usersFile` / `groupsFile` config keys.

### Configuration

Config file path: `cfg/grid-admin-plugin.conf` (libconfig format). `%(DIR)` expands to the directory containing the config file. Key settings under `smartmet.plugin.grid-admin`:

- `content-server.type` — `redis` | `postgresql` | `http` | `corba`
- `authenticationRequired` — enables login/session management
- `usersFile` / `groupsFile` — CSV files for user/group auth (in `cfg/`)

The config uses environment variables from `SMARTMET_ENV_FILE` for content server addresses (e.g. `REDIS_CONTENT_SERVER_PRIMARY_ADDRESS`, `REDIS_CONTENT_SERVER_PRIMARY_PORT`, `PG_CONTENT_SERVER_PRIMARY_CONNECTION_STRING`, `CORBA_CONTENT_SERVER_IOR`, `HTTP_CONTENT_SERVER_URL`).
