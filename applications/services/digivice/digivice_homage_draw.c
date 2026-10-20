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

// 1: horned canine in a white pelt with a zig-zag hem.
static void hm_gabumon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t w = (int32_t)((frame / 3) % 2);
    hm_ink(p, false);
    hm_ell(p, -2, 4, 7, 7); // body
    hm_ell(p, 3, -6, 6, 5); // head
    hm_ell(p, 8, -4, 3, 2); // snout
    hm_tri(p, 1, -10, 5, -10, 4, -15); // horn
    hm_box(p, -5, 10, 3, 3); // legs
    hm_box(p, 0, 10, 3, 3);
    hm_tri(p, -8, 5, -13, 8 + w, -8, 9); // tail
    hm_ink(p, true);
    hm_ell(p, -2, 3, 6, 6); // pelt
    hm_ink(p, false);
    for(int32_t i = 0; i < 4; i++) { // zig-zag hem
        hm_tri(p, -7 + i * 3, 6, -5 + i * 3, 10, -3 + i * 3, 6);
    }
    hm_eye(p, 3, -8, pose);
    hm_eye(p, 6, -8, pose);
    hm_ink(p, true);
    if(pose == DigivicePoseAttack) {
        hm_ell(p, 9, -3, 2, 2);
    } else {
        hm_line(p, 7, -3, 10, -3);
    }
    hm_ink(p, false);
}

// 2: round bird with a three-tuft crest, a hooked beak and a flapping wing arm.
static void hm_biyomon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t f = (int32_t)((frame / 2) % 2);
    hm_ink(p, false);
    hm_ell(p, 0, 3, 7, 8); // body
    hm_ell(p, 3, -6, 6, 5); // head
    hm_tri(p, 8, -7, 8, -3, 14, -5); // beak
    hm_tri(p, 0, -10, 2, -10, -1, -15); // crest
    hm_tri(p, 2, -10, 4, -10, 3, -16);
    hm_tri(p, 4, -10, 6, -10, 7, -14);
    hm_tri(p, -5, 0, -14, 6 - f * 2, -4, 6); // wing arm
    hm_line(p, -2, 10, -2, 12); // legs
    hm_line(p, 3, 10, 3, 12);
    hm_box(p, -4, 12, 4, 1);
    hm_box(p, 1, 12, 4, 1);
    hm_tri(p, -6, 8, -13, 12, -4, 12); // tail feathers
    hm_ink(p, true);
    hm_ell(p, 2, 5, 3, 5); // belly
    hm_eye(p, 2, -8, pose);
    hm_eye(p, 6, -8, pose);
    hm_ink(p, true);
    hm_line(p, 8, -5, 12, -5);
    if(pose == DigivicePoseAttack) hm_tri(p, 8, -5, 14, -5, 8, -2);
    hm_ink(p, false);
}

// 3: armoured beetle with two antennae, flickering wings and a split shell.
static void hm_tentomon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    bool flutter = (frame / 2) % 2;
    hm_ink(p, false);
    hm_ell(p, -1, 3, 9, 9); // shell
    hm_ell(p, 8, -3, 5, 4); // head
    hm_line(p, 6, -6, 4, -12); // antennae
    hm_line(p, 9, -6, 12, -12);
    hm_dot(p, 4, -12);
    hm_dot(p, 12, -12);
    hm_line(p, -6, 10, -8, 12); // legs
    hm_line(p, -1, 11, -1, 12);
    hm_line(p, 4, 10, 6, 12);
    hm_line(p, 5, 2, 9, 5); // arm
    if(flutter) {
        hm_ell(p, -8, -6, 6, 2);
        hm_ell(p, 0, -8, 6, 2);
    }
    hm_ink(p, true);
    hm_line(p, -1, -5, -1, 11); // shell split
    hm_ell(p, -5, 0, 2, 2); // spots
    hm_ell(p, 3, 0, 2, 2);
    hm_ell(p, -5, 6, 2, 2);
    hm_ell(p, 3, 6, 2, 2);
    hm_eye(p, 7, -5, pose);
    hm_ink(p, true);
    if(pose == DigivicePoseAttack) {
        hm_ell(p, 10, -1, 2, 1);
    } else {
        hm_line(p, 9, -1, 11, -1);
    }
    hm_ink(p, false);
}

// 4: plant sprite with a flower crown, a leaf skirt and swaying vine arms.
static void hm_palmon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t f = (int32_t)((frame / 3) % 2);
    static const int8_t petal[6][2] = {
        {-4, -11}, {-1, -14}, {5, -14}, {8, -11}, {5, -8}, {-1, -8}};
    hm_ink(p, false);
    for(int32_t i = 0; i < 6; i++) { // petals: solid and outlined, alternating
        hm_ell(p, petal[i][0], petal[i][1], 3, 2);
        if(i % 2) {
            hm_ink(p, true);
            hm_ell(p, petal[i][0], petal[i][1], 2, 1);
            hm_ink(p, false);
        }
    }
    hm_ell(p, 2, -4, 6, 5); // head
    hm_tri(p, -5, 12, 9, 12, 2, 0); // body cone
    hm_tri(p, -8, 8, -2, 12, -9, 12); // leaf skirt
    hm_tri(p, 12, 8, 6, 12, 13, 12);
    hm_line(p, -3, 3, -9, 5); // vine arms
    hm_dot(p, -9, 6);
    if(pose == DigivicePoseAttack) {
        hm_line(p, 7, 3, 17, 2);
        hm_dot(p, 17, 3);
    } else {
        hm_line(p, 7, 3, 13, 2 + f);
        hm_dot(p, 13, 3 + f);
    }
    hm_box(p, -3, 12, 3, 1); // feet
    hm_box(p, 3, 12, 3, 1);
    hm_ink(p, true);
    hm_ell(p, 2, -11, 2, 2); // flower centre
    hm_ell(p, 2, -4, 4, 3); // face
    hm_eye(p, 0, -5, pose);
    hm_eye(p, 4, -5, pose);
    hm_ink(p, false);
    hm_line(p, 1, -2, 3, -2);
}

// 7: cat with tall ears, white gloves and a ringed tail that sways.
static void hm_gatomon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t q = (int32_t)((frame / 3) % 4);
    int32_t f = q < 3 ? q : 1; // 0, 1, 2, 1
    hm_ink(p, false);
    hm_ell(p, 0, 4, 5, 8); // body
    hm_ell(p, 2, -6, 7, 6); // head
    hm_tri(p, -4, -9, 0, -11, -4, -17); // ears
    hm_tri(p, 4, -11, 8, -9, 9, -17);
    hm_box(p, -3, 10, 3, 3); // legs
    hm_box(p, 2, 10, 3, 3);
    for(int32_t i = 0; i < 5; i++) { // tail
        hm_ell(p, -5 - i * 3 / 2 - f * i / 4, 8 - i * 3, 1, 1);
    }
    if(pose == DigivicePoseAttack) {
        hm_box(p, 7, 1, 7, 5); // glove outline
    } else {
        hm_box(p, 3, 1, 6, 5);
    }
    hm_box(p, -9, 1, 5, 5);
    hm_ink(p, true);
    for(int32_t i = 1; i < 4; i++) { // tail rings
        hm_dot(p, -5 - i * 3 / 2 - f * i / 4, 8 - i * 3);
    }
    hm_box(p, -8, 2, 3, 3); // gloves
    if(pose == DigivicePoseAttack) {
        hm_box(p, 8, 2, 5, 3);
    } else {
        hm_box(p, 4, 2, 4, 3);
    }
    hm_ell(p, 2, -6, 5, 4); // face
    hm_eye(p, 0, -7, pose);
    hm_eye(p, 4, -7, pose);
    hm_ink(p, false);
    hm_dot(p, 6, -5); // nose
    hm_line(p, 8, -5, 12, -6); // whiskers
    if(pose == DigivicePoseAttack) {
        hm_dot(p, 13, 2); // claws
        hm_dot(p, 13, 3);
        hm_dot(p, 13, 4);
    }
}

// 8: round head with two very long ears, a small body and a forehead mark.
static void hm_terriermon(const HmPen* p, uint32_t frame, DigivicePose pose) {
    int32_t f = (int32_t)((frame / 3) % 2);
    hm_ink(p, false);
    hm_tri(p, -3, -6, -6, -4, -10, 10 + f); // ears
    hm_tri(p, 5, -6, 8, -4, 12, 10 + f);
    hm_ell(p, 1, -4, 7, 6); // head
    hm_ell(p, 1, 6, 5, 5); // body
    hm_ell(p, -5, 5, 2, 2); // arms
    hm_ell(p, 7, 5, 2, 2);
    hm_ell(p, -2, 12, 3, 1); // feet
    hm_ell(p, 4, 12, 3, 1);
    hm_tri(p, 0, -10, 3, -10, 1, -14); // forehead mark
    hm_ink(p, true);
    hm_ell(p, 3, -2, 3, 2); // muzzle
    hm_ell(p, 1, 7, 3, 3); // belly
    hm_eye(p, -1, -5, pose);
    hm_eye(p, 4, -5, pose);
    hm_ink(p, false);
    if(pose == DigivicePoseAttack) {
        hm_ell(p, 4, 0, 2, 2);
    } else {
        hm_line(p, 2, 0, 5, 0);
    }
}

static const HmDraw hm_table[DIGIVICE_HOMAGE_COUNT] = {
    [0] = hm_agumon,
    [1] = hm_gabumon,
    [2] = hm_biyomon,
    [3] = hm_tentomon,
    [4] = hm_palmon,
    [5] = hm_gomamon,
    [6] = hm_patamon,
    [7] = hm_gatomon,
    [8] = hm_terriermon,
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
