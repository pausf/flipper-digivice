#pragma once

#include <gui/canvas.h>

#include "digivice_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigivicePoseIdle,
    DigivicePoseAttack,
} DigivicePose;

/** Text helpers in the 4x6 HUD font; y is the top of the glyphs. */
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

const char* digivice_form_name(DigiviceForm form);
const char* digivice_type_name(DigiviceType type);

/** Full home screen. `now` is the RTC timestamp, drives the VIRUX countdown and alert. */
void digivice_draw_home(
    Canvas* canvas,
    const DigiviceState* state,
    uint32_t frame,
    uint32_t now,
    bool sd_ok);

/** Evolution cutscene; `frame` runs 0..DIGIVICE_EVO_FRAMES. */
void digivice_draw_evolution(Canvas* canvas, DigiviceForm from, DigiviceForm to, uint32_t frame);

#ifdef __cplusplus
}
#endif
