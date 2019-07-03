#! /bin/bash

set -e pipefail

test -n "$CC" || CC=cc

source=$(cat <<EOF
#include <stdio.h>

int main() {
  printf("Hello World");
  return 0;
}
EOF
)

run_code() {
  printf "$source" \
      | $CC $CFLAGS $LDFLAGS -xc - -o /dev/stdout \
      | ./pexec
}

if [[ $(run_code) = "Hello World" ]]; then
  echo >&2 "Test Passed!"
  exit 0
else
  echo >&2 "Test Failure!"
  exit 1
fi
