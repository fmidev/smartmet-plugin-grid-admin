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
8. [Configuration](#8-configuration)
9. [Common tasks](#9-common-tasks)
10. [Known pitfalls](#10-known-pitfalls)

---

## 1. What the plugin does

`grid-admin.so` registers the **private** URL `/grid-admin` and serves two
things from it:

* **The Content Server API over HTTP** (`?method=…`). Feeding systems and tools can
  read, and in principle write, the content registry (producers, generations,
  geometries, files, content) without linking grid-content themselves. Requests go
  either to the plugin's **own** connection to the master registry (Redis, PostgreSQL,
  CORBA or HTTP) or, with `source=engine`, to the grid engine's in-memory content cache.
* **An HTML admin browser** (`?page=…`): the start page, the loaded engines and
  plugins, the installed SmartMet packages, and, with `target=grid-engine`, the grid
  engine's own browser (configuration, mapping and Lua files, Content, Data and Query
  Server state, content lists, logs).

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
* **`make test`** runs `test/Makefile` if one exists. Today there is only
  `test/AuthGating.sh`, a shell test against a **running** server
  (`URL=http://host:port/grid-admin ./test/AuthGating.sh`). It checks that an
  unauthenticated `method=` request is rejected with 403. It prints `SKIP` and exits 0
  when no server is reachable.
* `make configtest` points at `cnf/crid-admin.conf.sample`, which does not exist, so it
  checks nothing. The real sample is `cfg/grid-admin-plugin.conf`.

## 3. Source files

| File | Contents |
|------|----------|
| `grid-admin/Plugin.{h,cpp}` | Plugin lifecycle, configuration, the content server backend, API dispatch, and the authentication gate for API calls. |
| `grid-admin/Browser.{h,cpp}` | Sessions, login and logout, the start, engines, plugins and software pages, and delegation to `Engine::Grid::Engine::browserRequest()`. |
| `cfg/grid-admin-plugin.conf`, `cfg/users.csv`, `cfg/groups.csv` | Sample configuration, users and groups. |

## 4. Startup

**Constructor**

1. It checks the server API version and registers `/grid-admin` with
   `addPrivateContentHandler()`.

   A **private** handler is left out of the server's URI list, which the frontends use
   for routing. Restrict access with `plugins.grid-admin.ip_filters` in the server
   configuration (see the spine developer guide, §7).
2. It reads the configuration: the backend type and its connection settings,
   `authenticationRequired`, `usersFile` and `groupsFile`.

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

   Both are created with read methods **on** and write methods **off**.
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

* **Write methods are always disabled** on master (see [§10](#10-known-pitfalls)).
  The message processor only dispatches `add…`, `delete…` and `set…` when write
  methods are enabled, so these calls are answered with `result=` `UNKNOWN_METHOD`, even
  for a logged-in user.
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

## 8. Configuration

All keys are under `smartmet.plugin.grid-admin`:

| Key | Meaning |
|-----|---------|
| `authenticationRequired` | Require login for the browser and for `method=` calls. |
| `usersFile`, `groupsFile` | The CSV files above. |
| `content-server.type` | `redis`, `postgresql`, `corba` or `http`. |
| `content-server.redis.address`, `.port`, `.tablePrefix`, `.secondaryPort`, `.password` | Redis connection. |
| `content-server.redis.secondartAddress` | Secondary Redis address. **Note the spelling**: the code reads this misspelt key (see [§10](#10-known-pitfalls)). |
| `content-server.redis.lockEnabled` | Redis write lock (the sample file writes `lockEnable`, which is not read). |
| `content-server.postgresql.primaryConnectionString`, `.secondaryConnectionString` | PostgreSQL connection. |
| `content-server.corba.ior`, `content-server.http.url` | Remote content server. |

The sample configuration takes the addresses from the environment file named by
`SMARTMET_ENV_FILE` (`REDIS_CONTENT_SERVER_PRIMARY_ADDRESS`, …). The configuration is
read once at startup.

## 9. Common tasks

### 9.1 Adding a browser page

Add a `page_xxx()` method to `Browser`, dispatch it in `Browser::requestHandler()`
after the login check, and add a link to it in `page_start()`. Store any new state in
the session with `sessionInfo.setAttribute("grid-admin", name, value)` and save it
with `updateSessionInfo()`, the way the existing pages do. Check permissions with
`sessionInfo.mUserInfo.hasPermission(...)`.

### 9.2 Exposing another Content Server method over HTTP

This is done in grid-content, not here: implement the method in
`ContentServer::HTTP::ServerInterface` (and the client). The plugin passes every
`method=` through unchanged.

## 10. Known pitfalls

* **Set `authenticationRequired = true` explicitly**, restrict the URL with
  `plugins.grid-admin.ip_filters`, and use your own users file, not the sample.
* **`readMethodsEnabled` / `writeMethodsEnabled` are ignored.** The sample
  configuration documents them, but the code hard-codes read = on, write = off.
* **The secondary Redis address key is misspelt.** The code reads
  `content-server.redis.secondartAddress`, so the documented `secondaryAddress`
  has no effect and the default `127.0.0.1` is used. Likewise, the sample's `lockEnable`
  is not the `lockEnabled` the code reads.
* **Configuration errors are swallowed.** A missing mandatory attribute, or an unknown
  `content-server.type`, builds an exception object but never throws it. With an
  unknown type, the plugin starts without a content server, and the first `method=`
  call without `source=engine` fails.
* **The grid engine is required.** `init()` dereferences the engine without checking
  it, so the plugin cannot run on a server without the grid engine.
* **Sessions are per process.** Behind a load balancer, a login on one backend is
  unknown to the others.
