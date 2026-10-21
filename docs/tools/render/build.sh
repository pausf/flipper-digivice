#!/bin/sh
# Builds the host renderer against the real digivice sources and u8g2, then writes the PNGs and
# GIFs into docs/images. Needs cc and python3 only. Usage: docs/tools/render/build.sh
set -e
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
svc="$root/applications/services/digivice"
app="$root/applications/main/digivice"
tmp="${TMPDIR:-/tmp}/digivice-render"
mkdir -p "$tmp/frames"
rm -f "$tmp"/frames/*.f

# The app files that render.c includes directly are not compiled a second time.
srcs="$svc/digivice_draw.c $svc/digivice_draw_world.c $svc/digivice_draw_city.c \
$svc/digivice_draw_scene.c $svc/digivice_draw_roster.c $svc/digivice_homage_draw.c \
$svc/digivice_i18n.c $svc/digivice_rules.c $svc/digivice_roster.c $svc/digivice_expedition.c \
$svc/digivice_battle.c $svc/digivice_evolution.c $svc/digivice_homage.c $svc/digivice_city.c \
$svc/digivice_moves.c $svc/digivice_duel.c \
$app/digivice_app_care.c $app/digivice_app_dex.c $app/digivice_app_expedition.c \
$app/digivice_app_feed.c $app/digivice_app_nido.c $app/digivice_app_profile.c \
$app/digivice_app_settings.c $app/digivice_app_train.c"
u8g2=""
for f in "$root"/lib/u8g2/*.c; do
    case "$f" in */u8g2_glue.c) ;; *) u8g2="$u8g2 $f" ;; esac
done

# u8g2 ships optional pieces (kerning tables, PBM capture) that the canvas never calls.
case "$(uname)" in Darwin) gc="-Wl,-dead_strip" ;; *) gc="-Wl,--gc-sections" ;; esac

cc -std=gnu11 -O1 -w -ffunction-sections $gc \
    -I "$here/shim" -I "$root/applications/services" -I "$app" -I "$root/lib/u8g2" \
    "$here/render.c" "$here/host_canvas.c" $srcs $u8g2 -o "$tmp/render"
"$tmp/render" "$tmp/frames"
python3 "$here/to_png.py" "$tmp/frames" "$root/docs/images"
