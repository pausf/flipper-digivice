#pragma once

#include <gui/canvas.h>

#include "digivice_rules.h"
#include "digivice_i18n.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigivicePoseIdle,
    DigivicePoseAttack,
    DigivicePoseSleep,
} DigivicePose;

/** Main screen icons, in screen order: top row then bottom row. */
typedef enum {
    DigiviceIconFeed,
    DigiviceIconTrain,
    DigiviceIconBattle,
    DigiviceIconClean,
    DigiviceIconLight,
    DigiviceIconCure,
    DigiviceIconProfile,
    DigiviceIconSettings,
    DigiviceIconCount,
} DigiviceIcon;

/** Text helpers in the 4x6 Latin-1 HUD font; y is the top of the glyphs. Text is UTF-8. */
void digivice_draw_text(Canvas* canvas, int32_t x, int32_t y, const char* text);
uint16_t digivice_draw_text_width(Canvas* canvas, const char* text);

void digivice_draw_fill_ellipse(Canvas* canvas, int32_t cx, int32_t cy, int32_t rx, int32_t ry);
void digivice_draw_fill_triangle(
    Canvas* canvas,
    int32_t ax,
    int32_t ay,
    int32_t bx,
    int32_t by,
    int32_t cx,
    int32_t cy);
void digivice_draw_ring(Canvas* canvas, int32_t cx, int32_t cy, int32_t rx, int32_t ry);
void digivice_draw_brackets(
    Canvas* canvas,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    int32_t len);

/** Perspective floor from `horizon` to the bottom edge, scrolling with `frame`. */
void digivice_draw_grid(Canvas* canvas, int32_t horizon, uint32_t frame);
/** XOR strips at pseudo-random rows: the corruption / hit glitch. */
void digivice_draw_glitch(Canvas* canvas, uint32_t seed, uint8_t strips);
/** One XOR scan line sweeping down the screen. */
void digivice_draw_scanline(Canvas* canvas, uint32_t frame);

/** Draws a form centred on (cx, cy). `silhouette` fills it solid black (evolution flashes). */
void digivice_draw_form(
    Canvas* canvas,
    DigiviceForm form,
    int32_t cx,
    int32_t cy,
    uint32_t frame,
    DigivicePose pose,
    bool silhouette);
void digivice_draw_virux(Canvas* canvas, int32_t cx, int32_t cy, uint32_t frame);

/** Segmented HUD HP box, 58x13, with the name and the percentage. */
void digivice_draw_hp_box(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    const char* name,
    uint16_t hp,
    uint16_t hp_max);

/** Segmented timing meter; the zone is drawn hollow and the cursor as a tall bar. */
void digivice_draw_meter(Canvas* canvas, int32_t x, int32_t y, uint8_t zone_start, uint8_t cursor);

/** 9x9 icon at (x, y). `selected` draws it white on a 13x11 black box. */
void digivice_draw_icon(Canvas* canvas, DigiviceIcon icon, int32_t x, int32_t y, bool selected);
/** 7x6 poop pile, top-left at (x, y); `count` piles 8 px apart, with rising stink. */
void digivice_draw_poops(Canvas* canvas, uint8_t count, int32_t x, int32_t y, uint32_t frame);
/** 7x6 sickness skull, top-left at (x, y). */
void digivice_draw_skull(Canvas* canvas, int32_t x, int32_t y);
/** 5x5 heart, top-left at (x, y). */
void digivice_draw_heart(Canvas* canvas, int32_t x, int32_t y);
void digivice_draw_sparkle(Canvas* canvas, int32_t x, int32_t y, int32_t size);
/** Eight sparkles on an ellipse around (cx, cy), turning with `phase` (16 steps per turn). */
void digivice_draw_sparkle_ring(
    Canvas* canvas,
    int32_t cx,
    int32_t cy,
    int32_t rx,
    int32_t ry,
    uint32_t phase);
/** 4x4 ordered dither in the current colour; level 0 draws nothing, 16 fills the box. */
void digivice_draw_dither(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    uint8_t level);
/** Three Z's drifting up and right from (x, y), in the current colour. */
void digivice_draw_zzz(Canvas* canvas, int32_t x, int32_t y, uint32_t frame);
/** White crescent moon and twinkling stars, for the dark band from `top` to `bottom`. */
void digivice_draw_night_sky(Canvas* canvas, int32_t top, int32_t bottom, uint32_t frame);
/** Lights-off night: dark dither over rows top..bottom plus the sky. */
void digivice_draw_dark(
    Canvas* canvas,
    int32_t top,
    int32_t bottom,
    uint8_t level,
    uint32_t frame);
/** Centred notice box with a 1 px frame, its top at y. */
void digivice_draw_label(Canvas* canvas, int32_t y, const char* text);
/** Clean wave at column x: a wavy black line that wipes everything to its right, top..bottom. */
void digivice_draw_sweep(Canvas* canvas, int32_t x, int32_t top, int32_t bottom, uint32_t frame);
/** Syringe pointing left with its needle tip at (x, y); `push` 0..8 drives the plunger in. */
void digivice_draw_syringe(Canvas* canvas, int32_t x, int32_t y, int32_t push);
/** Data chip centred on (cx, cy); each of the 3 bites shrinks it, 3 bites draw nothing. */
void digivice_draw_food(Canvas* canvas, int32_t cx, int32_t cy, uint8_t bites);
/** Dashed ellipse ring that rotates with `phase` (16 steps per turn). */
void digivice_draw_dash_ring(Canvas* canvas, int32_t cx, int32_t cy, int32_t r, uint32_t phase);

const char* digivice_form_name(DigiviceForm form);
const char* digivice_type_name(DigiviceType type);

/** Full home screen. `now` is the RTC timestamp: it drives the VIRUX countdown and sleep. */
void digivice_draw_home(
    Canvas* canvas,
    const DigiviceState* state,
    uint32_t frame,
    uint32_t now,
    bool sd_ok);

/** Evolution cutscene; `frame` runs 0..DIGIVICE_EVO_FRAMES, `top` is the banner's first row. */
void digivice_draw_evolution(
    Canvas* canvas,
    DigiviceForm from,
    DigiviceForm to,
    uint32_t frame,
    int32_t top,
    DigiviceLang lang);

#ifdef __cplusplus
}
#endif
