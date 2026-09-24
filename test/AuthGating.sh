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
  exit 0
fi

echo "FAIL: unauthenticated destructive method= request returned HTTP ${status} (expected 403)"
echo "      URL: ${PROBE}"
echo "      NOTE: a 403 is only expected when the server sets authenticationRequired=true."
echo "            If this server is configured with authenticationRequired=false the"
echo "            method= path is intentionally open and this test does not apply."
exit 1
