#! /bin/bash

set -e pipefail

fail() {
  echo >&2 "Test Failure:" $@
  (( x++ ))
}

runC() {
  $CC $CFLAGS $LDFLAGS -xc - -o /dev/stdout | ./pexec "$@"
}

cd "$(dirname "$0")"
test -n "$CC" || CC=cc
test_failures=0

source=$(cat <<EOF
  #include <stdio.h>

  int main(int argc, char **argv) {
    printf("Hello World %i %s %s", argc, argv[1], argv[2]);
    return 0;
  }
EOF
)

export DUMMY_VAR=42
test "$(./pexec < "$(which echo)" This is it)" = "This is it" || fail "Execute static file"
test "$(runC foo bar baz <<< "$source")" = "Hello World 4 foo bar" || fail "Execute fifo"
test "$(./pexec < "$(which env)" | grep DUMMY_VAR)" = "DUMMY_VAR=42" || fail "Static File Env"
test "$(cat "$(which env)" | ./pexec | grep DUMMY_VAR)" = "DUMMY_VAR=42" || fail "FIFO Env"

if [[ "$test_failures" -eq 0 ]]; then
  echo >&2 "All tests passed!"
else
  echo >&2 "$test_failures tests failed!"
  exit 1
fi
