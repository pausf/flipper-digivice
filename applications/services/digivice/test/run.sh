#!/bin/sh
# Builds and runs the host-side digivice rules test with the system compiler.
set -e
here="$(cd "$(dirname "$0")" && pwd)"
services="$here/../.."
out="${TMPDIR:-/tmp}/digivice_rules_test"
cc -std=c11 -Wall -Wextra -Werror -I "$services" \
    "$here/digivice_rules_test.c" "$here/../digivice_rules.c" -o "$out"
"$out"
