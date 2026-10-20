#pragma once

#include <gui/canvas.h>

#include "digivice_expedition.h"
#include "digivice_i18n.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigivicePoseIdle,
    DigivicePoseAttack,
    DigivicePoseSleep,
    DigivicePoseHurt, /**< trail monsters and bosses only */
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

/** Clipped primitives in the current colour: safe with any off-screen coordinate. */
void digivice_draw_line(Canvas* canvas, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
void digivice_draw_box(Canvas* canvas, int32_t x, int32_t y, int32_t w, int32_t h);
void digivice_draw_dot(Canvas* canvas, int32_t x, int32_t y);

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
/** Draws a roster form (digivice_draw_roster.c). False when the form is not a roster form. */
bool digivice_draw_roster(
    Canvas* canvas,
    DigiviceForm form,
    int32_t cx,
    int32_t cy,
    uint32_t frame,
    DigivicePose pose,
    bool silhouette);
void digivice_draw_virux(Canvas* canvas, int32_t cx, int32_t cy, uint32_t frame);

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
/** Meat or protein capsule centred on (cx, cy); each of the 3 bites shrinks it, 3 draw nothing. */
void digivice_draw_food_kind(
    Canvas* canvas,
    DigiviceFood food,
    int32_t cx,
    int32_t cy,
    uint8_t bites);
/** Dashed ellipse ring that rotates with `phase` (16 steps per turn). */
void digivice_draw_dash_ring(Canvas* canvas, int32_t cx, int32_t cy, int32_t r, uint32_t phase);

const char* digivice_form_name(DigiviceForm form);
const char* digivice_type_name(DigiviceType type);
/** Translated attribute name. */
const char* digivice_attr_name(DigiviceAttr attr, DigiviceLang lang);

// digivice_draw_world.c: trail monsters, bosses, projectiles and battle effects.

/** Trail monster centred on cx and standing on foot_y; `flip` makes it face left. */
void digivice_draw_foe(
    Canvas* canvas,
    DigiviceFoe foe,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    DigivicePose pose,
    bool flip,
    bool silhouette);
/** Area boss, at most 56x48, facing left. NEGATIVO (area 8) is `pet` drawn in negative. */
void digivice_draw_boss(
    Canvas* canvas,
    uint8_t area,
    DigiviceForm pet,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    bool silhouette);
/** Projectile centred on (x, y) flying towards `dir` (+1 right, -1 left). `size` is 2, 3, 5,
 * 7 (plus 2 for a Big charge) or DIGIVICE_SHOT_BEAM (a distinct
 * value: a 9 px beam, never a Big charge). The style follows the attribute. */
void digivice_draw_shot(
    Canvas* canvas,
    DigiviceAttr attr,
    int32_t x,
    int32_t y,
    uint8_t size,
    int8_t dir,
    uint32_t frame);
/** 7x7 attribute icon, top-left at (x, y): wave, bolt, keyhole square, slashed circle. */
void digivice_draw_attr_icon(Canvas* canvas, DigiviceAttr attr, int32_t x, int32_t y);
/** Eight rays from 0.45 r to r around (cx, cy). */
void digivice_draw_burst(Canvas* canvas, int32_t cx, int32_t cy, int32_t r);
void digivice_draw_star5(Canvas* canvas, int32_t cx, int32_t cy);
/** 5x9 shield, top-left at (x, y). */
void digivice_draw_shield(Canvas* canvas, int32_t x, int32_t y);
/** 9x13 speech bubble with a "!", top-left at (x, y). */
void digivice_draw_exclam_bubble(Canvas* canvas, int32_t x, int32_t y);
/** 7x7 bandage cross with a white outline, top-left at (x, y). */
void digivice_draw_bandage(Canvas* canvas, int32_t x, int32_t y);
/** XOR box: hit flashes and strobe frames. */
void digivice_draw_invert(Canvas* canvas, int32_t x, int32_t y, int32_t w, int32_t h);
/** 20-segment charge meter, 4 px segments on a 5 px pitch: zone hollow, cursor a tall bar. */
void digivice_draw_charge_meter(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    uint8_t zone_start,
    uint8_t zone_w,
    uint8_t cursor);
/** 9x9 round OK button, top-left at (x, y). */
void digivice_draw_ok_button(Canvas* canvas, int32_t x, int32_t y, bool down);
/** 7x5 arrow, top-left at (x, y); hollow unless `on`. */
void digivice_draw_arrow(Canvas* canvas, int32_t x, int32_t y, bool up, bool on);
/** 7 px hazard tape band from row y, scrolling with `frame`. */
void digivice_draw_hazard(Canvas* canvas, int32_t y, uint32_t frame);

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

// digivice_draw_scene.c: seeded area backgrounds, cached once per area.

#define DIGIVICE_SCENE_W     192
#define DIGIVICE_SCENE_DECOS 12

/** About 420 bytes: allocate it on the app heap, never on a stack. */
typedef struct {
    uint32_t seed;
    uint8_t area;
    uint8_t far[DIGIVICE_SCENE_W]; /**< top row of the far layer per column */
    uint8_t near[DIGIVICE_SCENE_W]; /**< top row of the near layer per column */
    uint8_t deco_x[DIGIVICE_SCENE_DECOS]; /**< decoration columns, 0..DIGIVICE_SCENE_W-1 */
    uint8_t deco_v[DIGIVICE_SCENE_DECOS]; /**< decoration variant 0..3 */
} DigiviceScene;

/** Builds the height profiles and decorations from the area's constant seed. */
void digivice_scene_init(DigiviceScene* scene, uint8_t area);
/** Draws the area background from row 9 down. `scroll` moves the near layer 1 px per unit;
 * `day_minute` (0..1439, from the RTC) picks the day, dusk or night sky. */
void digivice_draw_scene(
    Canvas* canvas,
    const DigiviceScene* scene,
    uint32_t scroll,
    uint32_t frame,
    uint16_t day_minute);

#ifdef __cplusplus
}
#endif
