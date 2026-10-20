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

static const HmDraw hm_table[DIGIVICE_HOMAGE_COUNT] = {0};

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
