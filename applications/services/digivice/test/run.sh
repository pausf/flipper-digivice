#!/bin/sh
# Builds and runs the host-side digivice tests with the system compiler.
set -e
here="$(cd "$(dirname "$0")" && pwd)"
services="$here/../.."
app="$services/../main/digivice"
out="${TMPDIR:-/tmp}"
flags="-std=c11 -Wall -Wextra -Werror -I $services"
rules="$here/../digivice_rules.c $here/../digivice_expedition.c $here/../digivice_battle.c"

for name in rules expedition battle; do
    # $rules is split on purpose: one argument per source file.
    cc $flags "$here/digivice_${name}_test.c" $rules -o "$out/digivice_${name}_test"
    "$out/digivice_${name}_test"
done

cc $flags "$here/digivice_i18n_test.c" "$here/../digivice_i18n.c" -o "$out/digivice_i18n_test"
# Every digivice source must stay ASCII-only: accents live in digivice_i18n.c as escapes.
"$out/digivice_i18n_test" "$here"/../*.c "$here"/../*.h "$here"/*.c "$app"/*.c "$app"/*.h \
    "$services/desktop/views/desktop_view_digivice.c"
