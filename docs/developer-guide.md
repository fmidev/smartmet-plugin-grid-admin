# grid-admin developer guide

This guide is for developers who change the `grid-admin` plugin
(`smartmet-plugin-grid-admin`). It describes what the plugin does, how a request flows
through it, how authentication works, and the pitfalls.

Background: the plugin exposes the grid-content **Content Server** API. See the
grid-content [developer guide](https://github.com/fmidev/smartmet-library-grid-content/blob/master/docs/developer-guide.md)
for the content model, the backends and the HTTP transport. The grid-engine
[developer guide](https://github.com/fmidev/smartmet-engine-grid/blob/master/docs/developer-guide.md)
describes the engine browser that this plugin hosts.

## Contents

1. [What the plugin does](#1-what-the-plugin-does)
2. [Building and testing](#2-building-and-testing)
3. [Source files](#3-source-files)
4. [Startup](#4-startup)
5. [Request flow](#5-request-flow)
6. [Content Server API](#6-content-server-api)
7. [Browser UI, sessions and authentication](#7-browser-ui-sessions-and-authentication)
8. [The new user interface (/grid-console)](#8-the-new-user-interface-grid-console)
9. [Configuration](#9-configuration)
10. [Common tasks](#10-common-tasks)
11. [Known pitfalls](#11-known-pitfalls)

---

## 1. What the plugin does

`grid-admin.so` registers two **private** URLs. `/grid-admin` serves two
things:

* **The Content Server API over HTTP** (`?method=…`). Feeding systems and tools can
  read, and in principle write, the content registry (producers, generations,
  geometries, files, content) without linking grid-content themselves. Requests go
  either to the plugin's **own** connection to the master registry (Redis, PostgreSQL,
  CORBA or HTTP) or, with `source=engine`, to the grid engine's in-memory content cache.
* **An HTML admin browser** (`?page=…`): the start page, the loaded engines and
  plugins, the installed SmartMet packages, and, with `target=grid-engine`, the grid
  engine's own browser (configuration, mapping and Lua files, Content, Data and Query
  Server state, content lists, logs).

`/grid-console` (configurable, see [section 8](#8-the-new-user-interface-grid-console)) serves the
new user interface: a page that reads and writes a JSON API and keeps its state in its
own address. The legacy pages stay at `/grid-admin` unchanged for a testing period.

The plugin has its own content server connection so that the API reflects the master
storage directly, not the engine cache, which lags behind by up to a second.

## 2. Building and testing

```bash
make               # builds grid-admin.so
make CORBA=disabled
make install       # -> $(plugindir)
make rpm
```

* CORBA is on by default; the stub include paths point at the installed grid-content
  headers.
* The plugin links grid-files, grid-content, spine and macgyver. The grid engine
  symbols are resolved at runtime.
* **`make test`** runs `test/Makefile`: `smartmet-plugin-test` requests of the
  `/grid-console` page, files and JSON API (`test/input`, expected answers in `test/output`)
  against the grid test data. `smartmet-grid-test-redis` (package
  `smartmet-library-grid-files-test`) starts the test Redis on a Unix socket. The tests
  need the grid engine >= 26.11.9-1 (`isBrowserEnabled()`, `getBrowserFlags()`); before
  it is installed, point the engine `libfile` in a copy of `test/cnf/reactor.conf` at a
  local build.
  POST requests are `.post` files with CRLF line ends, written byte for byte.
* `test/AuthGating.sh` is a separate shell test against a **running** server
  (`URL=http://host:port/grid-admin ./test/AuthGating.sh`). It checks that an
  unauthenticated `method=` request is rejected with 403. It prints `SKIP` and exits 0
  when no server is reachable.
* `make configtest` points at `cnf/crid-admin.conf.sample`, which does not exist, so it
  checks nothing. The real sample is `cfg/grid-admin-plugin.conf`.

## 3. Source files

| File | Contents |
|------|----------|
| `grid-admin/Plugin.{h,cpp}` | Plugin lifecycle, configuration, the content server backend, API dispatch, and the authentication gate for API calls. |
| `grid-admin/Browser.{h,cpp}` | Sessions, login and logout, the start, engines, plugins and software pages, and delegation to `Engine::Grid::Engine::browserRequest()`. The session functions of the new user interface (`openSession()`, `newLoginKey()`, `login()`, `logout()`, `canView()`, `isAdministrator()`). |
| `grid-admin/Ui.cpp` | The new user interface: serving its files and its JSON API. |
| `ui/` | The page: `index.html`, `grid-console.css` and ES modules (`main.js` routing, `overview.js`, `content.js`, `config.js`, `logs.js`, `login.js`, `edit.js` and `form.js` for the modifications, helpers). Installed to `/usr/share/smartmet/grid-admin`. |
| `test/` | The `smartmet-plugin-test` suite of `/grid-console`. |
| `cfg/grid-admin-plugin.conf`, `cfg/users.csv`, `cfg/groups.csv` | Sample configuration, users and groups. |

## 4. Startup

**Constructor**

1. It checks the server API version and registers `/grid-admin` with
   `addPrivateContentHandler()`.

   A **private** handler is left out of the server's URI list, which the frontends use
   for routing. Without `plugins.grid-admin.ip_filters` in the server configuration it
   accepts only localhost and the private networks 10.0.0.0/8, 172.16.0.0/12 and
   192.168.0.0/16. Setting `ip_filters` replaces that default (see the spine developer
   guide, §7).
2. It reads the configuration: the backend type and its connection settings,
   `authenticationRequired`, `usersFile`, `groupsFile`, `readMethodsEnabled` and
   `writeMethodsEnabled`.

**`init()`**

1. It creates the content server backend for `content-server.type`:
   * `redis`: `RedisImplementation` (address, port, table prefix, secondary, lock,
     password);
   * `postgresql`: `PostgresqlImplementation`;
   * `corba` / `http`: a client for a remote content server.
2. It gets the grid engine (`getEngine<Engine::Grid::Engine>("grid")`).
3. It initialises two `ContentServer::HTTP::ServerInterface` message processors:
   * `itsMessageProcessor1` over the plugin's own backend;
   * `itsMessageProcessor2` over `itsGridEngine->getContentServer_sptr()` (the cache).

   Both are created with the `readMethodsEnabled` and `writeMethodsEnabled` settings
   (by default read methods **on** and write methods **off**).
4. It initialises the `Browser` with the engine and the users and groups files.

**`shutdown()`** shuts down the plugin's own content server connection.

## 5. Request flow

```
requestHandler()
  ├─ method= present and authenticationRequired and not Browser::isAuthenticated()  → 403 "Authentication required"
  ├─ request()
  │    ├─ method= present → apiRequest()          (text/plain)
  │    └─ otherwise       → Browser::requestHandler()  (text/html)
  ├─ status 200 (handled) or 501 (not handled)
  ├─ Cache-Control: no-cache, Expires: now + 1 s
  └─ on exception: 400, first 300 characters of the error in X-Content-Error
```

## 6. Content Server API

`apiRequest()` turns the HTTP request into a grid-content `T::RequestMessage`:

* **GET** (no body): every query parameter becomes a `key=value` line, for example
  `/grid-admin?method=getProducerInfoList&sessionId=0`.
* **POST** (with a body): every non-empty line of the body is one `key=value` line.
  Use this for large requests such as `addFileInfoListWithContent`.

The message goes to `itsMessageProcessor2` if `source=engine`, otherwise to
`itsMessageProcessor1`. `ServerInterface::processRequest()` looks at `method`, calls
the corresponding `ContentServer::ServiceInterface` method, and writes the reply as
lines (`result=…`, then records as CSV). The plugin returns those lines as
`text/plain`, one per line. The format is the same one that
`ContentServer::HTTP::ClientImplementation` sends and parses, so the grid-content HTTP
client can talk to this plugin (the `http` content source type).

Two things to remember:

* **Write methods are disabled by default.**
  The message processor only dispatches `add…`, `delete…` and `set…` when
  `writeMethodsEnabled` is true. Otherwise these calls are answered with `result=`
  `UNKNOWN_METHOD`, even for a logged-in user. `readMethodsEnabled = false` does the same
  for the `get…` methods.
* **`source=engine` reads from the cache.** The cache follows the master through
  events, so a record written a moment ago can still be missing there.

## 7. Browser UI, sessions and authentication

`Browser::requestHandler()`:

1. **Session.** It reads the `sessionId` cookie. If there is no session, the session
   has expired, or it was created from another client IP, it creates a new session in
   `SessionManagement::localSessionManagement`. That store is in process memory:
   sessions are lost on restart and are not shared between servers behind a load
   balancer. The cookie is (re)sent with an expiry of one hour.
2. **Login.** When `authenticationRequired` is true and the session has no user, the
   request body is parsed as `username:hash`. The hash is
   `SHA1(password + password + password + sessionKey)`, where the session key is a
   random value that the login page embeds; the page computes the hash in JavaScript.
   The server looks the user up in `usersFile`, computes the same hash
   (`countHash()`), and stores the user in the session if they match. Until a user is
   logged in, every page request returns the login page.
3. **Logout** (`page=logout`) deletes the session and starts a new, anonymous one.
4. **Dispatch.** `target` and `page` are stored in the session. With
   `target=grid-engine`, the request goes to the engine's browser, provided the user has
   the `grid-content-view` permission. Otherwise `page` selects
   `start`, `engines`, `plugins` or `software`.

Set `authenticationRequired = true` in every deployment.

`Browser::isAuthenticated()` is the check used for API calls: a valid `sessionId`
cookie, a known session, the same client IP, and not expired.
API clients therefore have to log in through the browser flow first and send the
cookie.

Users and groups (`usersFile`, `groupsFile`) are CSV files:

```
# users:  userId;username;password;description;group[,group…]
1;alice;<password>;Administrator;grid-admin
# groups: group;permission[,permission…]
grid-admin;grid-content-view,grid-content-add,grid-content-delete,grid-content-modify
```

Make the files readable only by the server user.

## 8. The new user interface (/grid-console)

Everything is below one URL prefix, `ui.url` (default `/grid-console`):

| Path | Contents |
|------|----------|
| `/grid-console` | Redirect to `/grid-console/` so that the relative references of the page work. |
| `/grid-console/` | `index.html` |
| `/grid-console/ui/NAME` | A file of `ui.directory`. Only plain names (letters, digits, `-`, `_`, one `.`) with the extensions `js`, `css`, `html` and `svg` are served. The files are kept in memory and read again when their modification time changes, so the page can be edited without a restart. Served with an ETag and `Cache-Control: no-cache`. |
| `/grid-console/api/NAME` | The JSON API below. Answers have `Cache-Control: no-store`. |

The page has four views: an overview (key figures, content sources, the engine state
tree and the installed SmartMet software), the content browser and editor (source ›
producer › generation › file › messages), the configuration files and the logs. Its state is in the address (`#/content?producer=3&generation=4`),
so every view can be bookmarked and the back button works. The server only remembers the
login in the session. Message rows link to the grid-gui viewer (`ui.viewer`, default
`/gui`).

### API

| Call | Parameters | Answer |
|------|------------|--------|
| `session` | | Whether a login is required and done, the user, what the session may view and modify (`canView`, `canModify`, `canControlLogs`), and the legacy and viewer URLs. |
| `login` | GET: none. POST: `username:hash` | GET returns a one-time challenge `key`. POST logs in, as in the legacy login, and answers like `session`, or 403. |
| `logout` | POST | Ends the session. |
| `overview` | | The engine state as a tree (`getStateAttributes()`) and the content sources. |
| `producers` | `source` | The producers with their number of generations and newest analysis time. |
| `generations` | `producer`, `source` | The producer and its generations, newest first. With the cache, each generation also has its number of files. |
| `files` | `generation`, `after`, `limit` (1–1000, default 100), `source` | A page of files in id order from `after` (inclusive); `next` starts the next page or is null. With the cache, also the total `count`. |
| `contents` | `file`, `offset`, `limit` (1–5000, default 500), `source` | The file, its producer and generation, and a page of its messages in message order. |
| `config` | | The configured files in groups: the engine configuration, the producer search file, and the parameter mapping, producer mapping, parameter alias and Lua files. |
| `configfile` | `kind`, `index` | The text of one configured file (at most 4 MB). The file is chosen by its kind and index, never by a path. Every line mentioning a password has its value replaced. |
| `logs` | | The processing and debug logs of the content, data and query servers. |
| `log` | `server` (`content`, `data`, `query`), `type` (`processing`, `debug`), `lines` (1–10000) | The last lines of a log. Only regular files are read: a log written to `/dev/stdout` may be a pipe or a socket under systemd. |
| `software` | | The `libsmartmet*` libraries, engines and plugins with sizes and times. |
| `modify` | POST JSON, see below | Adds, updates or deletes a producer, generation, file or message. |
| `logcontrol` | POST JSON `{"server", "type", "action": "enable"\|"disable"\|"clear"}` | Controls a log. Only a regular file is cleared. |

`source` is `cache` (default, the engine's content cache) or the index of a content
source of the engine. Counting files in a Redis or PostgreSQL source could be slow, so
counts are given only for the cache.

Errors are JSON `{"error": "…"}` with status 400 (bad parameter, or a modification the
content server refused, with its reason), 404 (unknown record), 401 (login required),
403 (no permission, or a failed login) or 503 (`browser.enabled = false` in the grid
engine).

### Modifications

`POST api/modify` takes a JSON object:

```json
{"source": "0", "type": "generation", "action": "update", "generation": "13",
 "fields": {"status": "2", "deletion": "20261231T000000"}}
```

* `source` is the index of a content source of the engine. The cache cannot be
  modified: it follows the source, so the engine sees a change after its next content
  update.
* `type` is `producer`, `generation`, `file` or `content` (a message), and `action` is
  `add`, `update` or `delete`.
* The ids (`producer`, `generation`, `file`, `message`) name the record for update and
  delete, and the parent for add: a generation is added with its `producer`, a file
  with its `generation`, a message with its `file` and its `message` index.
* An update reads the record and changes only the given fields; the page sends only the
  fields that changed. Times are `YYYYMMDDTHHMMSS`; an empty `deletion` means never.
* Deleting a producer deletes its generations, files and messages; deleting a
  generation its files and messages; deleting a file its messages.

As in the grid engine browser, modifications need a logged-in member of the
`grid-admin` group and the corresponding bit in the grid engine's `browser.flags`
(`Engine::isBrowserEnabled()`, `getBrowserFlags()`): `contentModificationEnabled` (1)
for `modify`, `logModificationEnabled` (2) for `logcontrol`. Without authentication
nothing can be modified. `session` tells the page what it may do; the server checks
again for every call.

### Sessions and login

The new interface uses the same sessions as the legacy pages; the cookie has `Path=/`,
so a login in either is valid in both. The login protocol is the legacy one: the page
asks for a key with `GET api/login`, computes `h = SHA1("username:password")` and posts
`username:SHA1(h + h + h + key)`. Unlike the legacy page, the key is replaced after
every attempt, so a captured answer cannot be used again.

Login, logout, `modify` and `logcontrol` must be POST requests with the `X-Grid-Console`
header and `Content-Type: text/plain`. A cross-site form cannot set the header, and a cross-site
script cannot send it without a CORS preflight, which the server does not answer. The
cookie is also `SameSite=Strict`. `text/plain` is registered as an allowed POST content
type of the `/grid-console` handler; spine rejects other types.

With `authenticationRequired = false` everybody may view everything and nobody may
modify anything, as in the legacy pages.

## 9. Configuration

All keys are under `smartmet.plugin.grid-admin`:

| Key | Meaning |
|-----|---------|
| `authenticationRequired` | Require login for the browser and for `method=` calls (default `false`). |
| `usersFile`, `groupsFile` | The CSV files above. |
| `readMethodsEnabled`, `writeMethodsEnabled` | Which Content Server API methods `method=` may call (defaults `true` and `false`). |
| `content-server.type` | `redis`, `postgresql`, `corba` or `http`. |
| `content-server.redis.address`, `.port`, `.tablePrefix`, `.secondaryPort`, `.password` | Redis connection. |
| `content-server.redis.secondaryAddress` | Secondary Redis address. The misspelt `secondartAddress` of older configurations is read only when `secondaryAddress` is not set. |
| `content-server.redis.lockEnabled` | Redis write lock. |
| `content-server.postgresql.primaryConnectionString`, `.secondaryConnectionString` | PostgreSQL connection. |
| `content-server.corba.ior`, `content-server.http.url` | Remote content server. |
| `ui.url` | URL of the new user interface (default `/grid-console`, empty disables it). Must start with `/`, must not end with `/` and must differ from `/grid-admin`. |
| `ui.directory` | Directory of the files of the page (default `/usr/share/smartmet/grid-admin`). |
| `ui.viewer` | URL of the grid-gui viewer the message rows link to (default `/gui`, empty for no links). |

The sample configuration takes the addresses from the environment file named by
`SMARTMET_ENV_FILE` (`REDIS_CONTENT_SERVER_PRIMARY_ADDRESS`, …). The configuration is
read once at startup.

## 10. Common tasks

### 10.1 Adding a browser page

Add a `page_xxx()` method to `Browser`, dispatch it in `Browser::requestHandler()`
after the login check, and add a link to it in `page_start()`. Store any new state in
the session with `sessionInfo.setAttribute("grid-admin", name, value)` and save it
with `updateSessionInfo()`, the way the existing pages do. Check permissions with
`sessionInfo.mUserInfo.hasPermission(...)`.

### 10.2 Adding a view or an API call to /grid-console

Add an `api_xxx()` method to `Plugin` in `Ui.cpp` and register it in `ui_api()`. Read
calls get the request; throw `BadRequest`, `NotFound` or `Forbidden` for 400, 404 and
403. A call that changes something must be POST with the `X-Grid-Console` header (see how
`modify` is dispatched) and check the session itself. Add a request
and its expected answer to `test/input` and `test/output`. A new view is a module in
`ui/` that exports `renderXxx(root, params, ctx)` and returns a cleanup function; add it
to `views` in `main.js`. Build all text with `textContent` (the `el()` helper does),
never with `innerHTML`. List new files in the `install` target if they have a new
extension.

### 10.3 Exposing another Content Server method over HTTP

This is done in grid-content, not here: implement the method in
`ContentServer::HTTP::ServerInterface` (and the client). The plugin passes every
`method=` through unchanged.

## 11. Known pitfalls

* **Set `authenticationRequired = true` explicitly**, restrict the URL with
  `plugins.grid-admin.ip_filters`, and use your own users file, not the sample.
* **Sessions are per process.** Behind a load balancer, a login on one backend is
  unknown to the others.
