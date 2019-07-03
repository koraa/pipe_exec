#! /bin/bash

set -e
set -o pipefail

# Dealing with temp files
tmpfiles=()
cleanup() {
  rm -rf "${tmpfiles[@]}"
}
trap cleanup EXIT

fail() {
  echo >&2 "Test Failure:" $@
  (( x++ ))
}

export DUMMY_VAR=42
test "$(./pexec < "$(which echo)" This is it)" = "This is it" || fail "Execute static file"
test "$(cat "$(which echo)" | ./pexec foo bar baz)" = "foo bar baz" || fail "Execute fifo"
test "$(./pexec < "$(which env)" | grep DUMMY_VAR)" = "DUMMY_VAR=42" || fail "Static File Env"
test "$(cat "$(which env)" | ./pexec | grep DUMMY_VAR)" = "DUMMY_VAR=42" || fail "FIFO Env"

if [[ "$test_failures" -eq 0 ]]; then
  echo >&2 "All tests passed!"
else
  echo >&2 "$test_failures tests failed!"
  exit 1
fi
