#include "digivice_draw.h"
#include "digivice_config.h"
#include "digivice_roster.h"

#include <stdio.h>
#include <u8g2.h>

// sin(i * 22.5 deg) * 64, so rotations stay in integer math.
static const int8_t sin16[16] =
    {0, 24, 45, 59, 64, 59, 45, 24, 0, -24, -45, -59, -64, -59, -45, -24};
// Breathing offset over 8 frames.
static const int8_t wobble[8] = {0, 1, 1, 1, 0, -1, -1, -1};

static int32_t dv_sin(uint32_t i) {
    return sin16[i & 15];
}

static int32_t dv_cos(uint32_t i) {
    return sin16[(i + 4) & 15];
}

// Cheap deterministic hash so glitches look random but need no RNG.
static uint32_t dv_hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dUL;
    x ^= x >> 15;
    x *= 0x846ca68bUL;
    x ^= x >> 16;
    return x;
}

// u8g2 does not clip lines and its coordinates are unsigned 16 bit: a negative endpoint wraps to
// ~65535 and the line loops for ages. Every line goes through this Cohen-Sutherland clip.
static int dv_outcode(int32_t x, int32_t y) {
    return (x < 0 ? 1 : 0) | (x > 127 ? 2 : 0) | (y < 0 ? 4 : 0) | (y > 63 ? 8 : 0);
}

static bool dv_clip_line(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1) {
    int c0 = dv_outcode(*x0, *y0);
    int c1 = dv_outcode(*x1, *y1);
    while(c0 | c1) {
        if(c0 & c1) return false;
        int c = c0 ? c0 : c1;
        int32_t dx = *x1 - *x0;
        int32_t dy = *y1 - *y0;
        int32_t x, y;
        if(c & 8) {
            x = *x0 + dx * (63 - *y0) / dy;
            y = 63;
        } else if(c & 4) {
            x = *x0 + dx * (0 - *y0) / dy;
            y = 0;
        } else if(c & 2) {
            y = *y0 + dy * (127 - *x0) / dx;
            x = 127;
        } else {
            y = *y0 + dy * (0 - *x0) / dx;
            x = 0;
        }
        if(c == c0) {
            *x0 = x;
            *y0 = y;
            c0 = dv_outcode(x, y);
        } else {
            *x1 = x;
            *y1 = y;
            c1 = dv_outcode(x, y);
        }
    }
    return true;
}

static void dv_line(Canvas* canvas, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    if(dv_clip_line(&x0, &y0, &x1, &y1)) canvas_draw_line(canvas, x0, y0, x1, y1);
}

// Bitmaps from the approved mockup (v2-classic.html), one row per entry, leftmost pixel in the
// highest of the `w` bits.
static const uint16_t icons[DigiviceIconCount][9] = {
    [DigiviceIconFeed] = {0x070, 0x0F8, 0x1FC, 0x1FC, 0x0F8, 0x074, 0x006, 0x009, 0x006},
    [DigiviceIconTrain] = {0x000, 0x101, 0x183, 0x183, 0x1FF, 0x183, 0x183, 0x101, 0x000},
    [DigiviceIconBattle] = {0x101, 0x082, 0x044, 0x028, 0x010, 0x028, 0x145, 0x082, 0x145},
    [DigiviceIconClean] = {0x000, 0x010, 0x038, 0x07C, 0x07C, 0x038, 0x000, 0x155, 0x0AA},
    [DigiviceIconLight] = {0x038, 0x044, 0x082, 0x082, 0x044, 0x028, 0x038, 0x038, 0x010},
    [DigiviceIconCure] = {0x038, 0x028, 0x1EF, 0x101, 0x1EF, 0x028, 0x038, 0x000, 0x000},
    [DigiviceIconProfile] = {0x000, 0x004, 0x004, 0x024, 0x024, 0x124, 0x124, 0x1FF, 0x000},
    [DigiviceIconSettings] = {0x010, 0x0BA, 0x044, 0x082, 0x1C7, 0x082, 0x044, 0x0BA, 0x010},
};
static const uint16_t poop_rows[6] = {0x08, 0x18, 0x3C, 0x1C, 0x7E, 0x7F};
static const uint16_t skull_rows[6] = {0x3E, 0x7F, 0x55, 0x7F, 0x3E, 0x2A};
static const uint16_t heart_rows[5] = {0x0A, 0x1F, 0x1F, 0x0E, 0x04};
static const uint8_t bayer4[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

static void dv_dot(Canvas* canvas, int32_t x, int32_t y) {
    if(x >= 0 && x < 128 && y >= 0 && y < 64) canvas_draw_dot(canvas, x, y);
}

// canvas_draw_box takes unsigned sizes and wraps negative origins, so every box is clipped here.
static void dv_box(Canvas* canvas, int32_t x, int32_t y, int32_t w, int32_t h) {
    if(x < 0) {
        w += x;
        x = 0;
    }
    if(y < 0) {
        h += y;
        y = 0;
    }
    if(x + w > 128) w = 128 - x;
    if(y + h > 64) h = 64 - y;
    if(w > 0 && h > 0) canvas_draw_box(canvas, x, y, (size_t)w, (size_t)h);
}

static void
    dv_bitmap(Canvas* canvas, int32_t x, int32_t y, const uint16_t* rows, int32_t w, int32_t h) {
    for(int32_t j = 0; j < h; j++) {
        for(int32_t i = 0; i < w; i++) {
            if(rows[j] & (1U << (w - 1 - i))) dv_dot(canvas, x + i, y + j);
        }
    }
}

void digivice_draw_line(Canvas* canvas, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    dv_line(canvas, x0, y0, x1, y1);
}

void digivice_draw_box(Canvas* canvas, int32_t x, int32_t y, int32_t w, int32_t h) {
    dv_box(canvas, x, y, w, h);
}

void digivice_draw_dot(Canvas* canvas, int32_t x, int32_t y) {
    dv_dot(canvas, x, y);
}

void digivice_draw_text(Canvas* canvas, int32_t x, int32_t y, const char* text) {
    canvas_set_custom_u8g2_font(canvas, u8g2_font_4x6_tf);
    canvas_draw_str(canvas, x, y + 5, text);
}

uint16_t digivice_draw_text_width(Canvas* canvas, const char* text) {
    canvas_set_custom_u8g2_font(canvas, u8g2_font_4x6_tf);
    return canvas_string_width(canvas, text);
}

void digivice_draw_fill_ellipse(Canvas* canvas, int32_t cx, int32_t cy, int32_t rx, int32_t ry) {
    if(rx < 0 || ry < 0) return;
    if(ry == 0) {
        dv_line(canvas, cx - rx, cy, cx + rx, cy);
        return;
    }
    for(int32_t y = -ry; y <= ry; y++) {
        // Largest hw with (hw/rx)^2 + (y/ry)^2 <= 1.
        int32_t hw = rx;
        while(hw > 0 && hw * hw * ry * ry + y * y * rx * rx > rx * rx * ry * ry)
            hw--;
        dv_line(canvas, cx - hw, cy + y, cx + hw, cy + y);
    }
}

static int32_t dv_edge(int32_t px, int32_t py, int32_t qx, int32_t qy, int32_t x, int32_t y) {
    return (qx - px) * (y - py) - (qy - py) * (x - px);
}

static int32_t dv_min3(int32_t a, int32_t b, int32_t c) {
    int32_t m = a < b ? a : b;
    return m < c ? m : c;
}

static int32_t dv_max3(int32_t a, int32_t b, int32_t c) {
    int32_t m = a > b ? a : b;
    return m > c ? m : c;
}

void digivice_draw_fill_triangle(
    Canvas* canvas,
    int32_t ax,
    int32_t ay,
    int32_t bx,
    int32_t by,
    int32_t cx,
    int32_t cy) {
    for(int32_t y = dv_min3(ay, by, cy); y <= dv_max3(ay, by, cy); y++) {
        for(int32_t x = dv_min3(ax, bx, cx); x <= dv_max3(ax, bx, cx); x++) {
            int32_t w0 = dv_edge(bx, by, cx, cy, x, y);
            int32_t w1 = dv_edge(cx, cy, ax, ay, x, y);
            int32_t w2 = dv_edge(ax, ay, bx, by, x, y);
            if((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                dv_dot(canvas, x, y);
            }
        }
    }
}

void digivice_draw_ring(Canvas* canvas, int32_t cx, int32_t cy, int32_t rx, int32_t ry) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_fill_ellipse(canvas, cx, cy, rx, ry);
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_fill_ellipse(canvas, cx, cy, rx - 1, ry - 1);
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_brackets(
    Canvas* canvas,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    int32_t len) {
    dv_line(canvas, x0, y0, x0 + len - 1, y0);
    dv_line(canvas, x0, y0, x0, y0 + len - 1);
    dv_line(canvas, x1 - len + 1, y0, x1, y0);
    dv_line(canvas, x1, y0, x1, y0 + len - 1);
    dv_line(canvas, x0, y1, x0 + len - 1, y1);
    dv_line(canvas, x0, y1 - len + 1, x0, y1);
    dv_line(canvas, x1 - len + 1, y1, x1, y1);
    dv_line(canvas, x1, y1 - len + 1, x1, y1);
}

void digivice_draw_grid(Canvas* canvas, int32_t horizon, uint32_t frame) {
    int32_t depth = 63 - horizon;
    for(int32_t i = -8; i <= 8; i++) {
        dv_line(canvas, 64 + i * 4, horizon, 64 + i * 26, 63);
    }
    for(int32_t k = 0; k < 6; k++) {
        int32_t z = k * 8 + (int32_t)(frame % 8);
        dv_line(
            canvas,
            0,
            horizon + z * z * depth / (48 * 48),
            127,
            horizon + z * z * depth / (48 * 48));
    }
    dv_line(canvas, 0, horizon, 127, horizon);
}

void digivice_draw_glitch(Canvas* canvas, uint32_t seed, uint8_t strips) {
    canvas_set_color(canvas, ColorXOR);
    for(uint8_t k = 0; k < strips; k++) {
        uint32_t r = dv_hash(seed * 31 + k);
        int32_t y = (int32_t)(r % 60);
        int32_t h = 1 + (int32_t)((r >> 8) % 3);
        int32_t x = (int32_t)((r >> 12) % 96);
        int32_t w = 16 + (int32_t)((r >> 20) % 32);
        canvas_draw_box(canvas, x, y, w, h);
    }
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_scanline(Canvas* canvas, uint32_t frame) {
    int32_t y = (int32_t)((frame * 3) % 90);
    if(y >= 64) return;
    canvas_set_color(canvas, ColorXOR);
    dv_line(canvas, 0, y, 127, y);
    canvas_set_color(canvas, ColorBlack);
}

static void dv_eyes_and_mouth(
    Canvas* canvas,
    int32_t cx,
    int32_t by,
    int32_t k10,
    uint32_t frame,
    DigivicePose pose) {
    bool blink = (frame % 32) == 0 || pose == DigivicePoseSleep;
    for(int32_t sd = -1; sd <= 1; sd += 2) {
        int32_t ex = cx + 2 + sd * 3 * k10 / 10;
        int32_t ey = by - 1;
        if(blink) {
            dv_line(canvas, ex - 1, ey, ex + 1, ey);
        } else {
            canvas_draw_box(canvas, ex - 1, ey - 2, 3, 4);
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_dot(canvas, ex, ey - 1);
            canvas_set_color(canvas, ColorBlack);
        }
    }
    if(pose == DigivicePoseSleep) return;
    int32_t mx = cx + 1;
    int32_t my = by + 3;
    if(pose == DigivicePoseAttack) {
        canvas_draw_box(canvas, mx - 1, my, 5, 3);
    } else {
        canvas_draw_dot(canvas, mx - 1, my);
        canvas_draw_dot(canvas, mx, my + 1);
        canvas_draw_dot(canvas, mx + 1, my);
        canvas_draw_dot(canvas, mx + 2, my + 1);
        canvas_draw_dot(canvas, mx + 3, my);
    }
}

// Port of the mockup's mon(): the byte-sized dragon every rookie and champion form is built on.
static void dv_mon(
    Canvas* canvas,
    int32_t cx,
    int32_t cy,
    uint32_t frame,
    bool big,
    bool silhouette,
    DigivicePose pose) {
    int32_t k10 = big ? 14 : 10;
    int32_t br = pose == DigivicePoseSleep ? 0 : wobble[frame % 8];
    int32_t rx = 8 * k10 / 10;
    int32_t ry = 7 * k10 / 10 - (br > 0 ? 1 : 0);
    int32_t by = cy - (br > 0 ? 1 : 0);

    if(big) {
        int32_t f = pose == DigivicePoseSleep ? 0 : wobble[(frame * 2) % 8] * 2;
        for(int32_t sd = -1; sd <= 1; sd += 2) {
            digivice_draw_fill_triangle(
                canvas,
                cx + sd * rx / 2,
                by - 3,
                cx + sd * (rx + 11),
                by - 13 + f,
                cx + sd * (rx + 5),
                by + 5);
            if(!silhouette) {
                canvas_set_color(canvas, ColorWhite);
                digivice_draw_fill_triangle(
                    canvas,
                    cx + sd * (rx / 2 + 2),
                    by - 2,
                    cx + sd * (rx + 8),
                    by - 10 + f,
                    cx + sd * (rx + 4),
                    by + 2);
                canvas_set_color(canvas, ColorBlack);
                dv_line(
                    canvas, cx + sd * rx * 6 / 10, by - 2, cx + sd * (rx + 9), by - 11 + f);
            }
        }
    }

    // Tail.
    digivice_draw_fill_ellipse(canvas, cx - (rx + 1), by + 3, 3 * k10 / 10, 2 * k10 / 10);

    for(int32_t sd = -1; sd <= 1; sd += 2) {
        int32_t ex = cx + sd * 5 * k10 / 10;
        int32_t top = by - ry + 2;
        int32_t tip = big ? 9 : 5;
        digivice_draw_fill_triangle(
            canvas,
            ex - 3 * k10 / 10,
            top + 1,
            ex + 3 * k10 / 10,
            top + 1,
            ex + sd * 2 * k10 / 10,
            top - tip * k10 / 10);
        if(!silhouette && !big) {
            canvas_set_color(canvas, ColorWhite);
            digivice_draw_fill_triangle(canvas, ex - 1, top, ex + 1, top, ex + sd, top - 2);
            canvas_set_color(canvas, ColorBlack);
        }
        // Feet.
        digivice_draw_fill_ellipse(canvas, cx + sd * 4 * k10 / 10, cy + ry, 25 * k10 / 100, 1);
    }

    digivice_draw_fill_ellipse(canvas, cx, by, rx + 1, ry + 1);
    if(silhouette) return;

    canvas_set_color(canvas, ColorWhite);
    digivice_draw_fill_ellipse(canvas, cx, by, rx, ry);
    canvas_set_color(canvas, ColorBlack);
    dv_eyes_and_mouth(canvas, cx, by, k10, frame, pose);
}

static void dv_egg(Canvas* canvas, int32_t cx, int32_t cy, uint32_t frame, bool silhouette) {
    // The egg rocks every few seconds as it gets ready to hatch.
    int32_t rock = (frame % 40) < 6 ? wobble[(frame * 4) % 8] : 0;
    cx += rock;
    digivice_draw_fill_ellipse(canvas, cx, cy, 8, 10);
    if(silhouette) return;
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_fill_ellipse(canvas, cx, cy, 7, 9);
    canvas_set_color(canvas, ColorBlack);
    // Zigzag crack and two data spots.
    for(int32_t i = -6; i < 6; i += 2) {
        dv_line(canvas, cx + i, cy, cx + i + 1, cy - 2);
        dv_line(canvas, cx + i + 1, cy - 2, cx + i + 2, cy);
    }
    canvas_draw_box(canvas, cx - 4, cy - 6, 2, 2);
    canvas_draw_box(canvas, cx + 2, cy + 4, 3, 2);
}

static void dv_bitling(
    Canvas* canvas,
    int32_t cx,
    int32_t cy,
    uint32_t frame,
    bool silhouette,
    DigivicePose pose) {
    bool asleep = pose == DigivicePoseSleep;
    int32_t hop = (asleep || (frame % 8) < 4) ? 0 : 1;
    cy -= hop;
    digivice_draw_fill_triangle(canvas, cx - 5, cy - 3, cx - 2, cy - 4, cx - 6, cy - 8);
    digivice_draw_fill_triangle(canvas, cx + 5, cy - 3, cx + 2, cy - 4, cx + 6, cy - 8);
    digivice_draw_fill_ellipse(canvas, cx, cy, 7, 5);
    if(silhouette) return;
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_fill_ellipse(canvas, cx, cy, 6, 4);
    canvas_set_color(canvas, ColorBlack);
    bool blink = asleep || (frame % 32) == 0;
    canvas_draw_box(canvas, cx - 3, cy - 1, 2, blink ? 1 : 2);
    canvas_draw_box(canvas, cx + 2, cy - 1, 2, blink ? 1 : 2);
    dv_line(canvas, cx - 1, cy + 2, cx + 1, cy + 2);
}

// Champion emblems sit on the big mon's chest or head and tell the four types apart.
static void dv_emblem(Canvas* canvas, DigiviceForm form, int32_t cx, int32_t cy) {
    switch(form) {
    case DigiviceFormTagdrake:
        // NFC: antenna coil on the chest.
        canvas_draw_frame(canvas, cx - 3, cy + 5, 7, 4);
        canvas_draw_dot(canvas, cx, cy + 6);
        break;
    case DigiviceFormWavedrake:
        // Sub-GHz: a sine wave across the belly.
        for(int32_t i = -5; i <= 5; i++)
            canvas_draw_dot(canvas, cx + i, cy + 7 + (wobble[(i + 8) % 8] > 0 ? -1 : 1));
        break;
    case DigiviceFormBeamdrake:
        // IR: a visor over the eyes with a slit.
        canvas_draw_box(canvas, cx - 7, cy - 4, 15, 4);
        canvas_set_color(canvas, ColorWhite);
        dv_line(canvas, cx - 5, cy - 2, cx + 5, cy - 2);
        canvas_set_color(canvas, ColorBlack);
        break;
    case DigiviceFormPlugdrake:
        // USB: two plug prongs on the head.
        canvas_draw_box(canvas, cx - 3, cy - 15, 2, 4);
        canvas_draw_box(canvas, cx + 2, cy - 15, 2, 4);
        canvas_draw_box(canvas, cx - 4, cy - 12, 9, 2);
        break;
    default:
        break;
    }
}

static void dv_virubyte(
    Canvas* canvas,
    int32_t cx,
    int32_t cy,
    uint32_t frame,
    bool silhouette,
    DigivicePose pose) {
    // Spikes along the back, then the mon body filled black with hollow X eyes.
    for(int32_t i = -2; i <= 2; i++) {
        digivice_draw_fill_triangle(
            canvas,
            cx + i * 4 - 2,
            cy - 6,
            cx + i * 4 + 2,
            cy - 6,
            cx + i * 4,
            cy - 12 - (i & 1) * 2);
    }
    dv_mon(canvas, cx, cy, frame, false, true, pose);
    if(silhouette) return;
    canvas_set_color(canvas, ColorWhite);
    for(int32_t sd = -1; sd <= 1; sd += 2) {
        int32_t ex = cx + 2 + sd * 3;
        dv_line(canvas, ex - 1, cy - 3, ex + 1, cy - 1);
        dv_line(canvas, ex + 1, cy - 3, ex - 1, cy - 1);
    }
    if(pose == DigivicePoseAttack) {
        canvas_draw_box(canvas, cx, cy + 2, 5, 3);
    } else {
        dv_line(canvas, cx - 1, cy + 3, cx + 4, cy + 3);
    }
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_form(
    Canvas* canvas,
    DigiviceForm form,
    int32_t cx,
    int32_t cy,
    uint32_t frame,
    DigivicePose pose,
    bool silhouette) {
    canvas_set_color(canvas, ColorBlack);
    switch(form) {
    case DigiviceFormByteegg:
        dv_egg(canvas, cx, cy, frame, silhouette);
        break;
    case DigiviceFormBitling:
        dv_bitling(canvas, cx, cy, frame, silhouette, pose);
        break;
    case DigiviceFormBytemon:
        dv_mon(canvas, cx, cy, frame, false, silhouette, pose);
        break;
    case DigiviceFormTagdrake:
    case DigiviceFormWavedrake:
    case DigiviceFormBeamdrake:
    case DigiviceFormPlugdrake:
        dv_mon(canvas, cx, cy, frame, true, silhouette, pose);
        if(!silhouette) dv_emblem(canvas, form, cx, cy);
        break;
    case DigiviceFormVirubyte:
        dv_virubyte(canvas, cx, cy, frame, silhouette, pose);
        break;
    default:
        if(!digivice_draw_roster(canvas, form, cx, cy, frame, pose, silhouette)) {
            digivice_draw_ring(canvas, cx, cy, 8, 7);
        }
        break;
    }
}

// Port of the mockup's foe().
void digivice_draw_virux(Canvas* canvas, int32_t cx, int32_t cy, uint32_t frame) {
    int32_t f = wobble[(frame * 3) % 8] * 3;
    canvas_set_color(canvas, ColorBlack);
    for(int32_t sd = -1; sd <= 1; sd += 2) {
        digivice_draw_fill_triangle(
            canvas, cx + sd * 5, cy - 2, cx + sd * 17, cy - 7 + f, cx + sd * 12, cy + 5);
        digivice_draw_fill_triangle(
            canvas, cx + sd * 12, cy + 5, cx + sd * 15, cy + 1 + f / 2, cx + sd * 17, cy - 7 + f);
    }
    digivice_draw_fill_ellipse(canvas, cx, cy, 7, 6);
    digivice_draw_fill_triangle(canvas, cx - 2, cy - 5, cx - 4, cy - 11, cx - 5, cy - 4);
    digivice_draw_fill_triangle(canvas, cx + 2, cy - 5, cx + 4, cy - 11, cx + 5, cy - 4);
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_fill_triangle(canvas, cx - 5, cy - 2, cx - 1, cy, cx - 5, cy + 1);
    digivice_draw_fill_triangle(canvas, cx + 5, cy - 2, cx + 1, cy, cx + 5, cy + 1);
    dv_line(canvas, cx - 2, cy + 3, cx - 2, cy + 4);
    dv_line(canvas, cx + 2, cy + 3, cx + 2, cy + 4);
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_meter(Canvas* canvas, int32_t x, int32_t y, uint8_t zone_start, uint8_t cursor) {
    for(uint8_t i = 0; i < DIGIVICE_METER_SEGMENTS; i++) {
        int32_t sx = x + i * 5;
        if(digivice_rules_timing_hit(i, zone_start)) {
            canvas_draw_frame(canvas, sx, y, 4, 6);
        } else {
            canvas_draw_box(canvas, sx, y + 4, 4, 2);
        }
    }
    canvas_draw_box(canvas, x + cursor * 5, y - 3, 4, 10);
}

const char* digivice_form_name(DigiviceForm form) {
    const DigiviceRosterEntry* e = digivice_roster(form);
    return e ? e->name : "?";
}

const char* digivice_type_name(DigiviceType type) {
    static const char* const names[DigiviceTypeCount] = {"NFC", "SUBGHZ", "IR", "USB"};
    return type < DigiviceTypeCount ? names[type] : "---";
}

const char* digivice_attr_name(DigiviceAttr attr, DigiviceLang lang) {
    if(attr >= DigiviceAttrCount) attr = DigiviceAttrNulo;
    return digivice_str(lang, (DigiviceStr)(DigiviceStrAttrPortador + attr));
}

static void dv_vertical_meter(
    Canvas* canvas,
    int32_t x,
    int32_t bottom,
    const char* label,
    uint8_t value,
    uint8_t max) {
    digivice_draw_text(canvas, x, bottom - 4 * max - 6, label);
    for(uint8_t i = 0; i < max; i++) {
        int32_t sy = bottom - 3 - i * 4;
        if(i < value) {
            canvas_draw_box(canvas, x, sy, 3, 3);
        } else {
            dv_line(canvas, x, sy + 2, x + 2, sy + 2);
        }
    }
}

void digivice_draw_home(
    Canvas* canvas,
    const DigiviceState* state,
    uint32_t frame,
    uint32_t now,
    bool sd_ok) {
    // The desktop status bar owns rows 0..12, so the HUD starts below it.
    const int32_t horizon = 38;
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_grid(canvas, horizon, frame);

    // Data rain above the floor.
    for(int32_t i = 0; i < 9; i++) {
        int32_t x = 10 + i * 13;
        int32_t y = 13 + (int32_t)((frame * 2 + i * 17) % 25);
        canvas_draw_dot(canvas, x, y);
        if(y - 3 >= 13) canvas_draw_dot(canvas, x, y - 3);
    }

    DigiviceLang lang = (DigiviceLang)state->lang;
    bool asleep = digivice_rules_is_asleep(now);
    int32_t hy = 41 + wobble[(frame / 2) % 8];
    digivice_draw_ring(canvas, 64, 55, 15 + wobble[frame % 8], 3);
    digivice_draw_ring(canvas, 64, 55, 9, 2);
    digivice_draw_form(
        canvas,
        (DigiviceForm)state->form,
        64,
        hy,
        frame,
        asleep ? DigivicePoseSleep : DigivicePoseIdle,
        false);
    digivice_draw_brackets(canvas, 46, hy - 17, 82, hy + 10, 4);

    if(state->poop) {
        canvas_set_color(canvas, ColorWhite);
        dv_box(canvas, 83, 46, state->poop * 8 + 1, 8);
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_poops(canvas, state->poop, 84, 47, frame);
    }
    if(state->sick) digivice_draw_skull(canvas, 61, hy - 26 + wobble[(frame / 3) % 8]);
    if(asleep) {
        if(!state->light_on) digivice_draw_dark(canvas, 13, 56, 12, frame);
        canvas_set_color(canvas, state->light_on ? ColorBlack : ColorWhite);
        digivice_draw_zzz(canvas, 74, hy - 12, frame);
        canvas_set_color(canvas, ColorBlack);
        if(state->light_on) {
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_box(canvas, 43, 13, 62, 9);
            canvas_set_color(canvas, ColorBlack);
            digivice_draw_text(canvas, 45, 14, digivice_str(lang, DigiviceStrHomeLightsOff));
        }
    }

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 13, 40, 9);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_text(canvas, 3, 14, digivice_form_name((DigiviceForm)state->form));
    dv_line(canvas, 3, 21, 28, 21);

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 106, 13, 22, 24);
    canvas_set_color(canvas, ColorBlack);
    dv_vertical_meter(
        canvas,
        109,
        36,
        digivice_str(lang, DigiviceStrHomeMeterHunger),
        state->hunger,
        DIGIVICE_HUNGER_MAX);
    dv_vertical_meter(
        canvas,
        119,
        36,
        digivice_str(lang, DigiviceStrHomeMeterMood),
        state->mood,
        DIGIVICE_MOOD_MAX);

    // Fits a 6-glyph UTF-8 label (12 bytes), a space, a 32-bit count and "m".
    char text[32];
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 56, 128, 8);
    canvas_set_color(canvas, ColorBlack);
    snprintf(
        text,
        sizeof(text),
        "%s %lu",
        digivice_str(lang, DigiviceStrExp),
        (unsigned long)state->exp);
    digivice_draw_text(canvas, 2, 57, text);
    if(state->corruption > 0) {
        snprintf(text, sizeof(text), "C%u%%", (unsigned)state->corruption);
        digivice_draw_text(canvas, 96, 57, text);
    }
    if(!sd_ok) digivice_draw_text(canvas, 115, 57, "SD!");

    // Something waits on the trail: a blinking "!" over the pet and, for a fight, a label.
    if((digivice_rules_attention(state, now) || state->call_flags) && (frame % 10) < 6) {
        digivice_draw_exclam_bubble(canvas, 76, hy - 26);
        text[0] = '\0';
        if(digivice_rules_virux_active(state, now)) {
            uint32_t left_min = (state->virux_expires - now + 59) / 60;
            snprintf(
                text,
                sizeof(text),
                "%s %lum",
                digivice_str(lang, DigiviceStrHomeVirux),
                (unsigned long)left_min);
        } else if(digivice_expedition_trail_pending(state, now)) {
            snprintf(text, sizeof(text), "%s", digivice_str(lang, DigiviceStrHomeEncounter));
        } else if(
            digivice_expedition_boss_ready(state) && digivice_expedition_boss_gate_open(state)) {
            snprintf(text, sizeof(text), "%s", digivice_str(lang, DigiviceStrHomeBoss));
        } else if(state->call_flags) {
            snprintf(text, sizeof(text), "%s", digivice_str(lang, DigiviceStrHomeCall));
        }
        if(text[0]) {
            canvas_draw_box(canvas, 44, 56, 46, 8);
            canvas_set_color(canvas, ColorWhite);
            digivice_draw_text(canvas, 46, 57, text);
            canvas_set_color(canvas, ColorBlack);
        }
    }

    digivice_draw_scanline(canvas, frame);
    if(digivice_rules_glitchy(state) && (frame % 12) < 2) digivice_draw_glitch(canvas, frame, 3);
}

static void dv_sparkle(Canvas* canvas, int32_t x, int32_t y, int32_t size) {
    dv_line(canvas, x - size, y, x + size, y);
    dv_line(canvas, x, y - size, x, y + size);
}

void digivice_draw_evolution(
    Canvas* canvas,
    DigiviceForm from,
    DigiviceForm to,
    uint32_t frame,
    int32_t top,
    DigiviceLang lang) {
    const int32_t cx = 64;
    canvas_set_color(canvas, ColorBlack);

    if(frame < 20 || frame >= 30) {
        // Three dashed rings rotating in alternate directions.
        for(uint32_t r = 0; r < 3; r++) {
            int32_t rad = 18 + (int32_t)r * 12 + (int32_t)(frame % 12);
            for(uint32_t i = 0; i < 16; i++) {
                if((i + r) % 3 == 0) continue;
                uint32_t a = (r % 2) ? i + frame / 2 : i + 64 - (frame / 2) % 16;
                canvas_draw_dot(
                    canvas, cx + dv_cos(a) * rad * 3 / 128, 34 + dv_sin(a) * rad * 8 / 640);
            }
        }
    }

    if(frame < 20) {
        // The old form flickers into a silhouette, faster and faster.
        uint32_t period = 5 - frame / 5;
        bool sil = frame > 3 && (frame / period) % 2;
        digivice_draw_form(canvas, from, cx, 40, frame, DigivicePoseIdle, sil);
        uint32_t n = 3 + frame / 4;
        for(uint32_t i = 0; i < n; i++) {
            uint32_t a = frame + i * 16 / n;
            int32_t rr = 22 - (int32_t)frame * 8 / 20;
            dv_sparkle(
                canvas, cx + dv_cos(a) * rr * 14 / 640, 36 + dv_sin(a) * rr * 7 / 640, 1 + (i % 2));
        }
    } else if(frame < 30) {
        // Old and new silhouettes alternate under scan bars and glitch.
        bool alt = frame % 2;
        digivice_draw_form(
            canvas, alt ? to : from, cx, alt ? 42 : 40, frame, DigivicePoseIdle, true);
        canvas_set_color(canvas, ColorXOR);
        for(int32_t k = 0; k < 3; k++) {
            canvas_draw_box(canvas, 0, (int32_t)((frame * 20 + k * 25) % 64), 128, 3);
        }
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_glitch(canvas, frame, 6);
    } else {
        uint32_t t = frame - 30;
        digivice_draw_form(canvas, to, cx, 42, frame, DigivicePoseIdle, false);
        if(t < 16) {
            for(uint32_t i = 0; i < 7; i++) {
                uint32_t a = i * 2 + t;
                int32_t rr = 20 + (int32_t)t * 2;
                dv_sparkle(
                    canvas,
                    cx + dv_cos(a) * rr * 3 / 128,
                    38 + dv_sin(a) * rr * 7 / 640,
                    1 + (i % 2));
            }
        }
        // Banner slides in from the right, the name from the left.
        int32_t bx = t < 8 ? 128 - (int32_t)t * 16 : 0;
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, bx, top, 128, 11);
        canvas_set_color(canvas, ColorBlack);
        dv_line(canvas, bx, top, bx + 127, top);
        dv_line(canvas, bx, top + 10, bx + 127, top + 10);
        digivice_draw_text(canvas, bx + 38, top + 3, digivice_str(lang, DigiviceStrEvolution));
        int32_t nx = t < 8 ? -60 + (int32_t)t * 15 / 2 : 0;
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, nx + 36, 56, 56, 8);
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_text(canvas, nx + 42, 57, digivice_form_name(to));
        if(t < 2) digivice_draw_glitch(canvas, frame, 3);
    }
}

void digivice_draw_icon(Canvas* canvas, DigiviceIcon icon, int32_t x, int32_t y, bool selected) {
    if(icon >= DigiviceIconCount) return;
    canvas_set_color(canvas, ColorBlack);
    if(selected) {
        dv_box(canvas, x - 2, y - 1, 13, 11);
        canvas_set_color(canvas, ColorWhite);
    }
    dv_bitmap(canvas, x, y, icons[icon], 9, 9);
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_poops(Canvas* canvas, uint8_t count, int32_t x, int32_t y, uint32_t frame) {
    for(uint8_t i = 0; i < count; i++) {
        int32_t px = x + i * 8;
        dv_bitmap(canvas, px, y, poop_rows, 7, 6);
        // Two stink dots rising and resetting every 8 frames.
        int32_t q = (int32_t)((frame + i * 3) % 8);
        dv_dot(canvas, px + 1 + (q & 1), y - 1 - q);
        dv_dot(canvas, px + 5 - (q & 1), y - 1 - (q + 4) % 8);
    }
}

void digivice_draw_skull(Canvas* canvas, int32_t x, int32_t y) {
    dv_bitmap(canvas, x, y, skull_rows, 7, 6);
}

void digivice_draw_heart(Canvas* canvas, int32_t x, int32_t y) {
    dv_bitmap(canvas, x, y, heart_rows, 5, 5);
}

void digivice_draw_dither(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    uint8_t level) {
    for(int32_t j = y; j < y + h; j++) {
        for(int32_t i = x; i < x + w; i++) {
            if(bayer4[j & 3][i & 3] < level) dv_dot(canvas, i, j);
        }
    }
}

void digivice_draw_zzz(Canvas* canvas, int32_t x, int32_t y, uint32_t frame) {
    for(int32_t i = 0; i < 3; i++) {
        // Each Z climbs 16 px over 20 frames, a third of a cycle behind the previous one.
        int32_t q = (int32_t)((frame + (uint32_t)i * 7) % 20);
        digivice_draw_text(canvas, x + q * 14 / 20 + i * 2, y - q * 16 / 20, "Z");
    }
}

void digivice_draw_night_sky(Canvas* canvas, int32_t top, int32_t bottom, uint32_t frame) {
    // x = 92 keeps the moon clear of the home screen's meter box at x >= 106.
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_fill_ellipse(canvas, 92, top + 10, 5, 5);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_fill_ellipse(canvas, 94, top + 9, 4, 4);
    canvas_set_color(canvas, ColorWhite);
    for(int32_t i = 0; i < 6; i++) {
        if(dv_sin(frame / 2 + (uint32_t)i * 5) > 20) {
            int32_t sy = top + 3 + (i * 5) % 12;
            if(sy < bottom) dv_dot(canvas, (i * 23 + 7) % 120, sy);
        }
    }
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_dark(
    Canvas* canvas,
    int32_t top,
    int32_t bottom,
    uint8_t level,
    uint32_t frame) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_dither(canvas, 0, top, 128, bottom - top, level);
    if(level >= 12) digivice_draw_night_sky(canvas, top, bottom, frame);
}

void digivice_draw_label(Canvas* canvas, int32_t y, const char* text) {
    int32_t w = digivice_draw_text_width(canvas, text);
    int32_t x = 64 - w / 2;
    canvas_set_color(canvas, ColorWhite);
    dv_box(canvas, x - 3, y, w + 6, 8);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_frame(canvas, x - 3, y, w + 6, 8);
    digivice_draw_text(canvas, x, y + 1, text);
}

void digivice_draw_sweep(Canvas* canvas, int32_t x, int32_t top, int32_t bottom, uint32_t frame) {
    for(int32_t y = top; y < bottom; y++) {
        int32_t off = wobble[((uint32_t)y + frame * 2) % 8] * 2;
        canvas_set_color(canvas, ColorBlack);
        dv_box(canvas, x + off, y, 3, 1);
        canvas_set_color(canvas, ColorWhite);
        dv_box(canvas, x + off + 5, y, 128, 1);
    }
    canvas_set_color(canvas, ColorBlack);
    for(int32_t i = 0; i < 6; i++) {
        int32_t sy = top + 3 + (i * 7) % (bottom - top - 6);
        dv_sparkle(canvas, x + 8 + (i * 13) % 30, sy, 1);
    }
}

void digivice_draw_syringe(Canvas* canvas, int32_t x, int32_t y, int32_t push) {
    canvas_set_color(canvas, ColorBlack);
    dv_line(canvas, x, y, x + 6, y);
    canvas_draw_frame(canvas, x + 6, y - 3, 22, 6);
    dv_box(canvas, x + 8, y - 1, 14 - push, 2);
    dv_box(canvas, x + 28, y - 4, 2, 8);
    dv_box(canvas, x + 30 - push, y - 1, 6, 2);
}

void digivice_draw_food(Canvas* canvas, int32_t cx, int32_t cy, uint8_t bites) {
    if(bites >= 3) return;
    int32_t w = 10 - bites * 3;
    int32_t x = cx - w / 2;
    canvas_set_color(canvas, ColorBlack);
    dv_box(canvas, x, cy - 4, w, 8);
    canvas_set_color(canvas, ColorWhite);
    dv_box(canvas, x + 2, cy - 2, w - 4, 4);
    canvas_set_color(canvas, ColorBlack);
    // Chip pins above and below.
    for(int32_t p = 0; p < w; p += 3) {
        dv_box(canvas, x + p, cy - 6, 1, 2);
        dv_box(canvas, x + p, cy + 4, 1, 2);
    }
}

void digivice_draw_food_kind(
    Canvas* canvas,
    DigiviceFood food,
    int32_t cx,
    int32_t cy,
    uint8_t bites) {
    if(bites >= 3) return;
    canvas_set_color(canvas, ColorBlack);
    if(food == DigiviceFoodProtein) {
        // Capsule: two halves, one white. The mouth is on the left, so it is eaten from there.
        int32_t w = 14 - bites * 4;
        int32_t x = cx + 7 - w;
        dv_box(canvas, x + 2, cy - 4, w - 4 > 0 ? w - 4 : 1, 9);
        digivice_draw_fill_ellipse(canvas, x + 3, cy, 3, 4);
        digivice_draw_fill_ellipse(canvas, x + w - 4, cy, 3, 4);
        canvas_set_color(canvas, ColorWhite);
        dv_box(canvas, cx + 1, cy - 2, w / 2 > 1 ? w / 2 - 1 : 1, 5);
        canvas_set_color(canvas, ColorBlack);
        return;
    }
    // Meat: a round steak with a bone end, shrinking from the mouth side.
    int32_t rx = 5 - bites;
    digivice_draw_fill_ellipse(canvas, cx + 5 - rx, cy, rx, 4);
    canvas_set_color(canvas, ColorWhite);
    dv_box(canvas, cx + 6 - rx, cy - 1, 2, 2);
    canvas_set_color(canvas, ColorBlack);
    if(bites == 0) {
        dv_box(canvas, cx + 6, cy - 3, 3, 2);
        dv_box(canvas, cx + 6, cy + 2, 3, 2);
    }
}

void digivice_draw_sparkle(Canvas* canvas, int32_t x, int32_t y, int32_t size) {
    dv_sparkle(canvas, x, y, size);
}

void digivice_draw_sparkle_ring(
    Canvas* canvas,
    int32_t cx,
    int32_t cy,
    int32_t rx,
    int32_t ry,
    uint32_t phase) {
    for(uint32_t i = 0; i < 8; i++) {
        uint32_t a = i * 2 + phase;
        dv_sparkle(canvas, cx + dv_cos(a) * rx / 64, cy + dv_sin(a) * ry / 64, 1);
    }
}

void digivice_draw_dash_ring(Canvas* canvas, int32_t cx, int32_t cy, int32_t r, uint32_t phase) {
    for(uint32_t i = 0; i < 16; i += 2) {
        uint32_t a = i + phase;
        dv_line(
            canvas,
            cx + dv_cos(a) * r / 64,
            cy + dv_sin(a) * r * 8 / 640,
            cx + dv_cos(a + 1) * r / 64,
            cy + dv_sin(a + 1) * r * 8 / 640);
    }
}
