#include "digivice_draw.h"
#include "digivice_config.h"
#include "digivice_homage.h"
#include "digivice_pen.h"

#if DIGIVICE_EASTER_EGGS

// Partner sprites face right, 24 px tall around (cx, cy), feet at cy + 12. Companions draw the
// same code at half size: coordinates halve, sizes round up. The shared pen stays untouched.
typedef struct {
    DvPen pen;
    bool half;
} HmPen;

typedef void (*HmDraw)(const HmPen* p, uint32_t frame, DigivicePose pose);

static inline int32_t hs(const HmPen* p, int32_t v) {
    return p->half ? v / 2 : v;
}

static inline int32_t hz(const HmPen* p, int32_t v) {
    return p->half ? (v + 1) / 2 : v;
}

static inline void hm_ink(const HmPen* p, bool white) {
    pen_ink(&p->pen, white);
}

static inline void hm_ell(const HmPen* p, int32_t dx, int32_t dy, int32_t rx, int32_t ry) {
    pen_ell(&p->pen, hs(p, dx), hs(p, dy), hz(p, rx), hz(p, ry));
}

static inline void hm_tri(
    const HmPen* p,
    int32_t ax,
    int32_t ay,
    int32_t bx,
    int32_t by,
    int32_t cx,
    int32_t cy) {
    pen_tri(&p->pen, hs(p, ax), hs(p, ay), hs(p, bx), hs(p, by), hs(p, cx), hs(p, cy));
}

static inline void hm_line(const HmPen* p, int32_t ax, int32_t ay, int32_t bx, int32_t by) {
    pen_line(&p->pen, hs(p, ax), hs(p, ay), hs(p, bx), hs(p, by));
}

static inline void hm_box(const HmPen* p, int32_t dx, int32_t dy, int32_t w, int32_t h) {
    pen_box(&p->pen, hs(p, dx), hs(p, dy), hz(p, w), hz(p, h));
}

static inline void hm_dot(const HmPen* p, int32_t dx, int32_t dy) {
    pen_dot(&p->pen, hs(p, dx), hs(p, dy));
}

// At half size an eye is one white pixel; closed eyes (sleep) are lost, the zzz says it.
static inline void hm_eye(const HmPen* p, int32_t dx, int32_t dy, DigivicePose pose) {
    if(!p->half) {
        pen_eye(&p->pen, dx, dy, pose);
        return;
    }
    if(p->pen.sil) return;
    pen_ink(&p->pen, true);
    pen_dot(&p->pen, hs(p, dx), hs(p, dy));
}

// Stand-in for every partner that has no drawing yet: a ball with a spark.
static void hm_blob(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t b = (int32_t)((frame / 3) % 2);
    hm_ink(p, false);
    hm_ell(p, 0, 4, 9, 8 - b);
    hm_tri(p, -3, -3, 0, -10, 3, -3);
    hm_eye(p, -4, 0, pose);
    hm_eye(p, 2, 0, pose);
}

// 0: upright saurian with a big head, a long snout, a belly patch and a thick tail.
static void hm_agumon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t b = (int32_t)((frame / 3) % 2);
    hm_ink(p, false);
    hm_tri(p, -6, 3, -15, 9 + b, -6, 9); // tail
    hm_ell(p, -1, 4, 6, 7 - b); // body
    hm_ell(p, 3, -6, 7, 6); // head
    hm_ell(p, 9, -4, 4, 3); // snout
    hm_box(p, -5, 10, 4, 3); // feet
    hm_box(p, 0, 10, 4, 3);
    hm_line(p, 4, 1, 8, 3); // arm
    hm_dot(p, 9, 3);
    hm_dot(p, 9, 4);
    hm_ink(p, true);
    hm_ell(p, 1, 5, 3, 4); // belly
    hm_eye(p, 2, -8, pose);
    hm_eye(p, 6, -8, pose);
    hm_ink(p, true);
    if(pose == DigivicePoseAttack) {
        hm_ell(p, 9, -3, 3, 2);
    } else {
        hm_line(p, 6, -3, 11, -3);
    }
    hm_ink(p, false);
    hm_line(p, -1, 4, 3, 4); // belly stripes
    hm_line(p, -1, 6, 3, 6);
}

// 5: low seal with a spike mohawk, flippers and a fluke tail.
static void hm_gomamon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t w = (int32_t)((frame / 3) % 2);
    hm_ink(p, false);
    hm_tri(p, -12, 4, -17, -1 - w, -15, 8); // fluke
    hm_ell(p, -2, 5, 12, 6); // body
    hm_ell(p, 8, -1, 6, 5); // head
    for(int32_t i = 0; i < 4; i++) { // mohawk, back to front
        hm_tri(p, 3 + i * 2, -4, 5 + i * 2, -4, 4 + i * 2, -9 + (i % 2));
    }
    hm_ell(p, -7, 11, 3, 1); // flippers
    hm_ell(p, 4, 11, 3, 1);
    hm_line(p, 14, 0, 17, -1); // whiskers
    hm_line(p, 14, 2, 17, 3);
    hm_ink(p, true);
    hm_ell(p, 0, 8, 6, 3); // belly
    hm_line(p, -8, 2, -6, 5); // body markings
    hm_line(p, -4, 1, -2, 4);
    hm_eye(p, 7, -3, pose);
    if(pose == DigivicePoseAttack) {
        hm_ell(p, 13, 1, 2, 2);
    } else {
        hm_line(p, 11, 1, 14, 1);
    }
    hm_ink(p, false);
}

// 6: flying ball with big bat-wing ears that flap.
static void hm_patamon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t fl = (int32_t)((frame / 2) % 2) * 2;
    hm_ink(p, false);
    hm_tri(p, -4, -6, -14, -11 - fl, -11, 0); // left ear
    hm_tri(p, -11, 0, -8, 5, -4, 1);
    hm_tri(p, 4, -6, 14, -11 - fl, 11, 0); // right ear
    hm_tri(p, 11, 0, 8, 5, 4, 1);
    hm_ell(p, 0, 3, 8, 8); // body
    hm_ell(p, -4, 11, 3, 1); // feet
    hm_ell(p, 4, 11, 3, 1);
    hm_ink(p, true);
    hm_ell(p, 0, 7, 4, 3); // belly
    hm_eye(p, -4, -1, pose);
    hm_eye(p, 2, -1, pose);
    hm_ink(p, true);
    if(pose == DigivicePoseAttack) {
        hm_ell(p, 0, 4, 3, 2);
    } else {
        hm_line(p, -2, 3, 2, 3);
    }
    hm_ink(p, false);
}

static const HmDraw hm_table[DIGIVICE_HOMAGE_COUNT] = {
    [0] = hm_agumon,
    [5] = hm_gomamon,
    [6] = hm_patamon,
};

bool digivice_draw_homage(
    Canvas* canvas,
    uint8_t id,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    DigivicePose pose,
    bool flip,
    bool silhouette,
    bool half) {
    if(id >= DIGIVICE_HOMAGE_COUNT) return false;
    int32_t bob = pose == DigivicePoseIdle && (frame / 3) % 2 ? 1 : 0;
    int32_t back = pose == DigivicePoseHurt ? 2 : 0;
    HmPen p = {
        .pen =
            {
                .canvas = canvas,
                .cx = flip ? cx + back : cx - back,
                .cy = foot_y - (half ? 6 : 12) + bob,
                .flip = flip,
                .sil = silhouette,
            },
        .half = half,
    };
    HmDraw draw = hm_table[id] ? hm_table[id] : hm_blob;
    draw(&p, frame, pose);
    canvas_set_color(canvas, ColorBlack);
    return true;
}

#else

bool digivice_draw_homage(
    Canvas* canvas,
    uint8_t id,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    DigivicePose pose,
    bool flip,
    bool silhouette,
    bool half) {
    (void)canvas;
    (void)id;
    (void)cx;
    (void)foot_y;
    (void)frame;
    (void)pose;
    (void)flip;
    (void)silhouette;
    (void)half;
    return false;
}

#endif
