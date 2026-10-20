#!/bin/sh
# Builds and runs the host-side digivice tests with the system compiler.
set -e
here="$(cd "$(dirname "$0")" && pwd)"
services="$here/../.."
app="$services/../main/digivice"
out="${TMPDIR:-/tmp}"
flags="-std=c11 -Wall -Wextra -Werror -I $services"
rules="$here/../digivice_rules.c $here/../digivice_roster.c $here/../digivice_expedition.c $here/../digivice_battle.c $here/../digivice_evolution.c $here/../digivice_homage.c $here/../digivice_city.c"

for name in rules expedition battle roster evolution homage city; do
    # $rules is split on purpose: one argument per source file.
    cc $flags "$here/digivice_${name}_test.c" $rules -o "$out/digivice_${name}_test"
    "$out/digivice_${name}_test"
done

for name in rules expedition battle homage city; do
    # The partners compile out: every suite must still pass.
    cc $flags -DDIGIVICE_EASTER_EGGS=0 "$here/digivice_${name}_test.c" $rules \
        -o "$out/digivice_${name}_off_test"
    "$out/digivice_${name}_off_test"
done

cc $flags "$here/digivice_i18n_test.c" "$here/../digivice_i18n.c" -o "$out/digivice_i18n_test"
# Every digivice source must stay ASCII-only: accents live in digivice_i18n.c as escapes.
"$out/digivice_i18n_test" "$here"/../*.c "$here"/../*.h "$here"/*.c "$app"/*.c "$app"/*.h \
    "$services/desktop/views/desktop_view_digivice.c"
