#!/bin/sh
#
# Integration/regression test for the grid-admin Content Server API (method=) auth gate.
#
# Security regression (C-3): when the plugin is configured with
# authenticationRequired=true, an unauthenticated request carrying a method=
# parameter must be REJECTED (HTTP 403), not executed. Previously any such
# request was dispatched straight to the full Content Server API with no
# authentication, allowing e.g.
#     GET /grid-admin?method=deleteProducerInfoByName&sessionId=1&producerName=X
# to wipe metadata, or method=addFileInfo to register attacker-controlled paths
# (arbitrary file read / SSRF).
#
# This test requires a running smartmetd that loads the grid-admin plugin with
# authenticationRequired=true. Point it at that server via:
#     URL=http://host:port/grid-admin ./AuthGating.sh
# If no server is reachable the test SKIPS (exit 0) rather than failing, so it
# can be dropped into environments without a live server.

URL="${URL:-http://localhost:8080/grid-admin}"

# Any destructive Content Server method works as the probe; deleteProducerInfoByName
# is the one called out in the vulnerability report.
PROBE="${URL}?method=deleteProducerInfoByName&sessionId=1&producerName=__authgating_probe__"

# Reachability / skip check.
if ! curl -s -o /dev/null --max-time 5 "${URL}" ; then
  echo "SKIP: no server reachable at ${URL} (set URL=... to run this test)"
  exit 0
fi

# An unauthenticated method= request must be forbidden (403).
status=$(curl -s -o /dev/null -w '%{http_code}' --max-time 10 "${PROBE}")

if [ "${status}" = "403" ]; then
  echo "PASS: unauthenticated method=deleteProducerInfoByName rejected with HTTP ${status}"
else
  echo "FAIL: unauthenticated destructive method= request returned HTTP ${status} (expected 403)"
  echo "      URL: ${PROBE}"
  echo "      NOTE: a 403 is only expected when the server sets authenticationRequired=true."
  echo "            If this server is configured with authenticationRequired=false the"
  echo "            method= path is intentionally open and this test does not apply."
  exit 1
fi

# The browser session cookie must be HttpOnly and SameSite=Strict, and the
# interface must not allow cross-origin scripting via CORS.
headers=$(curl -s -D - -o /dev/null --max-time 10 "${URL}")

if echo "${headers}" | grep -qi '^Access-Control-Allow-Origin:'; then
  echo "FAIL: grid-admin sends an Access-Control-Allow-Origin header"
  exit 1
fi
echo "PASS: no Access-Control-Allow-Origin header"

cookie=$(echo "${headers}" | grep -i '^Set-Cookie: *sessionId=')
if [ -n "${cookie}" ]; then
  if echo "${cookie}" | grep -qi 'HttpOnly' && echo "${cookie}" | grep -qi 'SameSite=Strict'; then
    echo "PASS: session cookie is HttpOnly and SameSite=Strict"
  else
    echo "FAIL: session cookie lacks HttpOnly/SameSite=Strict: ${cookie}"
    exit 1
  fi
fi
exit 0
