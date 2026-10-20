#pragma once

// Procedural sprite pen shared by the trail monsters (digivice_draw_world.c) and the roster
// (digivice_draw_roster.c). Offsets are from (cx, cy), mirrored with `flip`.

#include "digivice_draw.h"

// Sprite pen: offsets from (cx, cy), mirrored with `flip`, all black with `sil`.
typedef struct {
    Canvas* canvas;
    int32_t cx;
    int32_t cy;
    bool flip;
    bool sil;
} DvPen;

static inline int32_t pen_x(const DvPen* p, int32_t dx) {
    return p->flip ? p->cx - dx : p->cx + dx;
}

static inline void pen_ink(const DvPen* p, bool white) {
    canvas_set_color(p->canvas, white && !p->sil ? ColorWhite : ColorBlack);
}

static inline void pen_ell(const DvPen* p, int32_t dx, int32_t dy, int32_t rx, int32_t ry) {
    digivice_draw_fill_ellipse(p->canvas, pen_x(p, dx), p->cy + dy, rx, ry);
}

static inline void pen_tri(
    const DvPen* p,
    int32_t ax,
    int32_t ay,
    int32_t bx,
    int32_t by,
    int32_t cx,
    int32_t cy) {
    digivice_draw_fill_triangle(
        p->canvas, pen_x(p, ax), p->cy + ay, pen_x(p, bx), p->cy + by, pen_x(p, cx), p->cy + cy);
}

static inline void pen_line(const DvPen* p, int32_t ax, int32_t ay, int32_t bx, int32_t by) {
    digivice_draw_line(p->canvas, pen_x(p, ax), p->cy + ay, pen_x(p, bx), p->cy + by);
}

static inline void pen_box(const DvPen* p, int32_t dx, int32_t dy, int32_t w, int32_t h) {
    int32_t x = p->flip ? p->cx - dx - w + 1 : p->cx + dx;
    digivice_draw_box(p->canvas, x, p->cy + dy, w, h);
}

static inline void pen_dot(const DvPen* p, int32_t dx, int32_t dy) {
    digivice_draw_dot(p->canvas, pen_x(p, dx), p->cy + dy);
}

// White 2x2 eye with a black pupil; an X while hurt.
static inline void pen_eye(const DvPen* p, int32_t dx, int32_t dy, DigivicePose pose) {
    if(p->sil) return;
    if(pose == DigivicePoseSleep) {
        pen_ink(p, true);
        pen_line(p, dx, dy + 1, dx + 1, dy + 1);
        return;
    }
    if(pose == DigivicePoseHurt) {
        pen_ink(p, true);
        pen_line(p, dx - 1, dy - 1, dx + 2, dy + 2);
        pen_line(p, dx - 1, dy + 2, dx + 2, dy - 1);
        return;
    }
    pen_ink(p, true);
    pen_box(p, dx, dy, 2, 2);
    pen_ink(p, false);
    pen_dot(p, dx + 1, dy + 1);
}
