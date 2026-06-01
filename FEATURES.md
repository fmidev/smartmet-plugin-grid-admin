# smartmet-plugin-grid-admin — Feature List

A structured inventory of capabilities provided by the grid-admin
plugin. Use as a checklist when drafting release notes. When new
functionality is added, append the new entry under the matching section
(and bump the *Last updated* line at the bottom).

`smartmet-plugin-grid-admin` is a SmartMet Server plugin (`grid-admin.so`)
that exposes the Content Information Storage to external systems and to
human operators. It serves two surfaces from the same endpoint:

- a **machine-facing HTTP API** for feeding systems and tooling, and
- a **browser-facing HTML admin UI** for human inspection of engines,
  plugins, and grid-engine state.

The plugin registers as a **private** content handler at `/grid-admin`
(`isAdminQuery() == true`).

---

## 1. Endpoints & request routing

A single URL prefix (`/grid-admin`) multiplexes two surfaces, decided by
which query parameter is present:

- **`/grid-admin?method=...`** — Content Server HTTP API call (machine
  interface).
- **`/grid-admin?page=...&target=...`** — Browser UI page (human
  interface).

## 2. Content Server HTTP API

A direct passthrough to the `ContentServer::HTTP::ServerInterface` so
external clients can read and update the Content Information Storage
without going through the grid engine.

- **Full Content Server method surface** — every `add*`, `delete*`,
  `set*`, `get*` method documented in
  `smartmet-library-grid-content/FEATURES.md` §2.
- **Case-insensitive method dispatch** (e.g. `method=getProducerInfoList`).
- **JSON-over-HTTP wire format** as defined by `ServerInterface`.

## 3. Source selection (direct vs. cached)

The plugin holds two message processors and a `source` parameter picks
between them:

- **default** (`source` absent) — query the direct backend (Redis /
  PostgreSQL / CORBA / HTTP).
- **`source=engine`** — query the grid engine's in-memory cached Content
  Server (faster, eventually consistent).

## 4. Content Server backends

Selected at startup by `content-server.type` in the plugin config.

- **Redis** (default) — primary + optional secondary, configurable table
  prefix, optional distributed locking.
- **PostgreSQL** — primary + optional secondary connection strings.
- **HTTP** — proxy a remote Content Server over JSON-over-HTTP.
- **CORBA** — proxy a remote Content Server over CORBA (omniORB).
- **Endpoints from environment** — `REDIS_CONTENT_SERVER_PRIMARY_ADDRESS`,
  `PG_CONTENT_SERVER_PRIMARY_CONNECTION_STRING`, `CORBA_CONTENT_SERVER_IOR`,
  `HTTP_CONTENT_SERVER_URL`, etc., expanded from `SMARTMET_ENV_FILE`.

## 5. Browser UI pages

Generated server-side by `Browser` from raw HTML. Pages:

- **`page=start`** — landing page.
- **`page=engines`** — list of loaded SmartMet engines with summary
  state.
- **`page=plugins`** — list of loaded plugins.
- **`page=software`** — installed SmartMet packages and versions.
- **`page=login`** — authentication form (only shown when auth is
  required).
- **`page=logout`** — session teardown.

### Grid-engine browser delegation

When `target=grid-engine` is set, the request is forwarded to
`Engine::Grid::Engine::browserRequest()`, exposing all grid-engine
admin pages from a single URL: producer/generation/file/content
listings, log viewers, parameter mapping editors, Lua script viewer,
etc. (see `smartmet-engine-grid/FEATURES.md` §9).

## 6. Authentication

Optional; controlled by `authenticationRequired` in the config.

- **Session cookies** — `sessionId` cookie tracks an in-process session;
  sessions expire after **1 hour**.
- **Challenge–response login** — the login page generates a per-session
  random key, embeds a JavaScript SHA1 implementation, and sends
  `username:SHA1(password+password+password+key)` over XHR POST.
- **Server-side verification** — `Browser::countHash()` re-computes the
  keyed SHA1 with the stored password.
- **User store** — CSV-backed (`usersFile`, `groupsFile`); reloaded by
  `Browser::readUsers()`.
- **Group-based authorization** — `grid-content-view` permission gates
  access to the grid-engine browser.
- **Unauthenticated state** — `UserInfo::getUserId() == 0`.

## 7. Session management

- **In-process** session storage (not shared across restarts).
- **Sliding** 1-hour expiry on activity.
- **Per-session user info** stored after login.
- **Logout** clears the session entry server-side.

## 8. Integration with the SmartMet stack

- **Engine dependency** — `smartmet-engine-grid` (resolved at runtime by
  the server; used only for the browser UI, not for API requests).
- **Library dependencies** — `smartmet-library-grid-content`,
  `smartmet-library-grid-files`, `smartmet-library-spine`,
  `smartmet-library-macgyver`.
- **Direct content-server connection** — the API surface does **not**
  go through the grid engine, so it can be used in deployments that
  have a Content Server but no grid engine.

## 9. Configuration

- **libconfig** format (`cfg/grid-admin-plugin.conf`) with SmartMet
  extensions (`@include`, `@ifdef`, `$(VAR)`, `%(DIR)`).
- **Key settings** under `smartmet.plugin.grid-admin`:
  - `content-server.type` — `redis` | `postgresql` | `http` | `corba`
  - `authenticationRequired` — enables login flow.
  - `usersFile` / `groupsFile` — CSV files in `cfg/`.
- **Environment-driven endpoints** — addresses pulled from
  `SMARTMET_ENV_FILE`.
- **Config validation** via `make configtest` (`cfgvalidate`).

## 10. Build & integration

- **Output**: `grid-admin.so`.
- **Loads at**: `$(prefix)/share/smartmet/plugins/grid-admin.so`.
- **Build**: `make`.
- **Skip CORBA**: `make CORBA=disabled`.
- **Install**: `make install`.
- **RPM**: `make rpm`.
- **Doxygen HTML**: `make doc`.
- **CI**: CircleCI on RHEL 8 / RHEL 10 via `ci-build deps` / `ci-build
  rpm` in the `fmidev/smartmet-cibase-{8,10}` Docker images. No test
  suite in this repo.

---

*Last updated: 2026-06-01.*
