# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

The **grid-admin** plugin for SmartMet Server. It provides an HTTP interface to the Content Information Storage, allowing external systems (e.g. feeding systems) to query and update content information. It also serves a simple HTML browser UI for inspecting server state (engines, plugins, installed software).

The plugin connects directly to the Content Server using its own configuration — it does **not** use the Grid Engine for content server access, though it does use the Grid Engine for browser UI features.

## Build commands

```bash
make                # Build grid-admin.so
make clean          # Clean build artifacts
make install        # Install to $(plugindir)
make rpm            # Build RPM package
make test           # Run tests (currently no test suite exists)
make format         # No-op — clang-format is disabled (.clang-format sets DisableFormat: true)
```

CORBA support is enabled by default. To disable: `make CORBA=disabled`.

## Dependencies

- **Libraries**: `smartmet-library-grid-files`, `smartmet-library-grid-content`, `smartmet-library-spine`, `smartmet-library-macgyver`
- **Engine**: `smartmet-engine-grid` (resolved at runtime by the server)
- **System**: `gdal`, `libconfig` (configpp), `omniORB4` (when CORBA enabled)
- **pkg-config**: `REQUIRES = gdal configpp`

## Architecture

The plugin has only two source files:

- **Plugin.cpp/.h** — Standard SmartMet plugin lifecycle (`init`, `shutdown`, `requestHandler`). Registers as a **private** content handler at `/grid-admin`. Routes requests: if a `method` parameter is present, delegates to the Content Server HTTP API (`apiRequest`); otherwise delegates to the `Browser` for HTML UI pages. Supports four content server backends: Redis (default), PostgreSQL, HTTP, and CORBA.
- **Browser.cpp/.h** — Generates HTML pages for browsing server state (start, engines, plugins, software). Handles session management (cookies), authentication (optional, password hashing), and delegates grid-engine browsing to `Engine::Grid::Engine::browserRequest()`.

### Request routing

1. `GET /grid-admin?method=...` — Content Server HTTP API call. The `source=engine` parameter routes through the Grid Engine's cached content server; otherwise routes to the direct content server (Redis/PostgreSQL/CORBA/HTTP).
2. `GET /grid-admin?page=...&target=...` — Browser UI. Pages: `start`, `engines`, `plugins`, `software`, `login`, `logout`. When `target=grid-engine`, delegates to the grid engine's browser.

### Configuration

Config file path: `cfg/grid-admin-plugin.conf` (libconfig format). Key settings under `smartmet.plugin.grid-admin`:
- `content-server.type` — `redis` | `postgresql` | `http` | `corba`
- `authenticationRequired` — enables login/session management
- `usersFile` / `groupsFile` — CSV files for user/group auth (in `cfg/`)
