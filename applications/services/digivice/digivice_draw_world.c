#include "digivice_draw.h"
#include "digivice_config.h"
#include "digivice_pen.h"

// cos and sin of i * 45 deg, x64, for the 8-ray effects.
static const int8_t cos8[8] = {64, 45, 0, -45, -64, -45, 0, 45};
static const int8_t sin8[8] = {0, 45, 64, 45, 0, -45, -64, -45};

static uint32_t dw_hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dUL;
    x ^= x >> 15;
    return x;
}

static void foe_estatik(const DvPen* p, uint32_t frame, DigivicePose pose) {
    pen_ink(p, false);
    // Static hair: three spikes that crackle on alternate frames.
    int32_t f = (int32_t)(frame % 2);
    pen_tri(p, -5, -3, -3, -12 + f, -1, -4);
    pen_tri(p, -1, -4, 1, -13 - f, 3, -4);
    pen_tri(p, 3, -4, 6, -11 + f, 6, -2);
    // Broken wings behind.
    pen_tri(p, -6, -1, -12, -7, -10, 2);
    pen_tri(p, -10, 2, -13, 4, -8, 4);
    pen_ell(p, 0, 2, 7, 7);
    pen_line(p, -6, 7, -11, 10);
    pen_tri(p, -13, 8, -10, 12, -14, 12);
    pen_box(p, -4, 8, 2, 4);
    pen_box(p, 2, 8, 2, 4);
    pen_eye(p, 2, -1, pose);
    pen_eye(p, 5, -1, pose);
    pen_ink(p, true);
    if(pose == DigivicePoseAttack) {
        pen_ell(p, 4, 5, 2, 1);
    } else {
        pen_line(p, 3, 5, 6, 5);
    }
    pen_ink(p, false);
}

static void foe_chipko(const DvPen* p, uint32_t frame, DigivicePose pose) {
    pen_ink(p, false);
    pen_box(p, -8, -1, 16, 9);
    pen_box(p, 6, -5, 7, 7);
    pen_line(p, -8, 4, -12, 7);
    pen_line(p, -12, 7, -11, 10);
    for(int32_t x = -6; x <= 3; x += 3)
        pen_box(p, x, -3 - (int32_t)((frame / 3 + (uint32_t)(x + 6)) % 2), 2, 2);
    pen_box(p, -6, 8, 2, 4);
    pen_box(p, 3, 8, 2, 4);
    pen_eye(p, 9, -4, pose);
    pen_ink(p, true);
    pen_line(p, 8, 1, pose == DigivicePoseAttack ? 11 : 12, pose == DigivicePoseAttack ? 0 : 1);
    if(!p->sil) {
        // The chip's die on the back.
        pen_box(p, -3, 1, 6, 4);
        pen_ink(p, false);
        pen_box(p, -2, 2, 4, 2);
    }
    pen_ink(p, false);
}

static void foe_korno(const DvPen* p, uint32_t frame, DigivicePose pose) {
    pen_ink(p, false);
    int32_t wag = (int32_t)(frame % 4 < 2);
    pen_line(p, -9, 2, -12, -1 - wag);
    pen_ell(p, -1, 4, 8, 6);
    pen_ell(p, 5, -3, 6, 5);
    pen_tri(p, 6, -7, 8, -14, 9, -6);
    pen_tri(p, 1, -6, -2, 0, 2, -2);
    pen_box(p, -6, 8, 2, 4);
    pen_box(p, 3, 8, 2, 4);
    pen_eye(p, 7, -5, pose);
    pen_ink(p, true);
    // The wave on its chest.
    pen_line(p, 0, 5, 1, 3);
    pen_line(p, 1, 3, 2, 5);
    pen_line(p, 2, 5, 3, 3);
    if(pose == DigivicePoseAttack) pen_ell(p, 9, 0, 2, 1);
    pen_ink(p, false);
    pen_dot(p, 11, -3);
}

static void foe_mokobit(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t squash = (int32_t)(frame % 6 < 3);
    pen_ink(p, false);
    pen_ell(p, 0, 3 + squash, 10, 9 - squash);
    pen_ink(p, true);
    pen_ell(p, 0, 3 + squash, 7, 6 - squash);
    pen_ink(p, false);
    if(pose == DigivicePoseHurt) {
        pen_line(p, -4, 0, -1, 3);
        pen_line(p, 2, 0, 5, 3);
    } else {
        pen_box(p, -3, 0, 2, 3);
        pen_box(p, 2, 0, 2, 3);
    }
    if(pose == DigivicePoseAttack) pen_ell(p, 0, 6, 2, 1);
    // Bubbles rising off its back.
    int32_t rise = (int32_t)(frame % 8);
    if(!p->sil) {
        digivice_draw_ring(p->canvas, pen_x(p, 8), p->cy - 8 - rise / 2, 2, 2);
        pen_dot(p, 11, -3 - rise / 3);
    }
}

static void foe_fuzzcan(const DvPen* p, uint32_t frame, DigivicePose pose) {
    pen_ink(p, false);
    for(int32_t x = -12; x <= 4; x += 4)
        pen_tri(p, x, -1, x + 2, -7 - (int32_t)((frame + (uint32_t)x) % 2), x + 4, -1);
    pen_ell(p, -2, 4, 12, 7);
    pen_ell(p, 9, -4, 7, 6);
    pen_box(p, 13, -3, 5, 4);
    pen_tri(p, 6, -8, 9, -15, 11, -8);
    pen_tri(p, 9, -15, 13, -13, 11, -11);
    pen_box(p, -11, 9, 3, 7);
    pen_box(p, -5, 10, 3, 6);
    pen_box(p, 3, 10, 3, 6);
    pen_box(p, 8, 9, 3, 7);
    pen_line(p, -14, 2, -18, -2);
    pen_eye(p, 9, -6, pose);
    pen_ink(p, true);
    if(pose == DigivicePoseAttack) {
        pen_box(p, 13, 0, 5, 2);
    }
    if(!p->sil) {
        // TV noise crawling over the fur.
        for(uint32_t i = 0; i < 6; i++) {
            uint32_t h = dw_hash(frame / 2 * 7 + i);
            pen_dot(p, -10 + (int32_t)(h % 18), 1 + (int32_t)((h >> 8) % 7));
        }
    }
    pen_ink(p, false);
}

static void foe_vampixel(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t flap = (int32_t)(frame % 4 < 2) * 4;
    pen_ink(p, false);
    pen_tri(p, -3, -4, -16, -10 + flap, -12, 6);
    pen_tri(p, 3, -4, 16, -10 + flap, 12, 6);
    pen_ink(p, true);
    pen_tri(p, -12, 6, -10, 2, -8, 6);
    pen_tri(p, 12, 6, 10, 2, 8, 6);
    pen_ink(p, false);
    pen_ell(p, 0, 0, 6, 7);
    pen_tri(p, -3, -6, -2, -13, 0, -6);
    pen_tri(p, 1, -6, 3, -13, 4, -6);
    pen_line(p, -2, 7, -3, 16);
    pen_line(p, 2, 7, 3, 16);
    pen_eye(p, -2, -3, pose);
    pen_eye(p, 2, -3, pose);
    pen_ink(p, true);
    pen_line(p, 1, 3, 1, pose == DigivicePoseAttack ? 6 : 4);
    pen_line(p, 3, 3, 3, pose == DigivicePoseAttack ? 6 : 4);
    pen_ink(p, false);
    // It glitches: one sliced row every few frames.
    if(!p->sil && frame % 8 == 0) {
        digivice_draw_invert(p->canvas, p->cx - 16, p->cy - 2 + (int32_t)(frame % 5), 32, 2);
    }
}

static void foe_silbapo(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t sac = (int32_t)(frame % 10 < 5 ? frame % 5 : 4 - frame % 5);
    pen_ink(p, false);
    pen_ell(p, -10, 14, 4, 2);
    pen_ell(p, 8, 14, 4, 2);
    pen_ell(p, 0, 6, 14, 9);
    pen_ell(p, 4, -3, 3, 3);
    pen_ell(p, 10, -2, 3, 3);
    pen_eye(p, 4, -4, pose);
    pen_eye(p, 10, -3, pose);
    pen_ell(p, 10, 9, 3 + sac, 2 + sac / 2);
    pen_ink(p, true);
    pen_ell(p, 10, 9, 1 + sac, sac / 2);
    pen_dot(p, -6, 4);
    pen_dot(p, -2, 8);
    pen_dot(p, -9, 9);
    pen_dot(p, 2, 2);
    if(pose == DigivicePoseAttack) pen_line(p, 6, 4, 13, 4);
    pen_ink(p, false);
}

static void foe_chatarro(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t sway = (int32_t)(frame % 6 < 3);
    pen_ink(p, false);
    pen_box(p, -8, -6, 16, 16);
    pen_box(p, -5, -14, 10, 8);
    pen_line(p, -8, -4, -14, 5 + sway);
    pen_line(p, -14, 5 + sway, -16, 9 + sway);
    pen_line(p, -14, 5 + sway, -12, 9 + sway);
    pen_box(p, 8, -4 + sway, 5, 12);
    pen_box(p, -6, 10, 4, 6);
    pen_box(p, 2, 10, 4, 6);
    pen_ink(p, true);
    pen_box(p, -6, -4, 12, 12);
    pen_box(p, -3, -11, 6, 2);
    pen_ink(p, false);
    if(pose == DigivicePoseHurt) {
        pen_line(p, -3, -12, 2, -10);
    } else {
        pen_dot(p, pose == DigivicePoseAttack ? 2 : 1, -11);
    }
    pen_dot(p, -3, -1);
    pen_dot(p, 3, 5);
    pen_dot(p, 10, 0 + sway);
}

void digivice_draw_foe(
    Canvas* canvas,
    DigiviceFoe foe,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    DigivicePose pose,
    bool flip,
    bool silhouette) {
    bool big = digivice_expedition_foe_champion(foe);
    int32_t bob = pose == DigivicePoseIdle && (frame / 3) % 2 ? 1 : 0;
    int32_t back = pose == DigivicePoseHurt ? 2 : 0;
    DvPen p = {
        .canvas = canvas,
        .cx = flip ? cx + back : cx - back,
        .cy = foot_y - (big ? 16 : 12) + bob,
        .flip = flip,
        .sil = silhouette,
    };
    switch(foe) {
    case DigiviceFoeEstatik:
        foe_estatik(&p, frame, pose);
        break;
    case DigiviceFoeChipko:
        foe_chipko(&p, frame, pose);
        break;
    case DigiviceFoeKorno:
        foe_korno(&p, frame, pose);
        break;
    case DigiviceFoeMokobit:
        foe_mokobit(&p, frame, pose);
        break;
    case DigiviceFoeFuzzcan:
        foe_fuzzcan(&p, frame, pose);
        break;
    case DigiviceFoeVampixel:
        foe_vampixel(&p, frame, pose);
        break;
    case DigiviceFoeSilbapo:
        foe_silbapo(&p, frame, pose);
        break;
    case DigiviceFoeChatarro:
        foe_chatarro(&p, frame, pose);
        break;
    default:
        break;
    }
    canvas_set_color(canvas, ColorBlack);
}

// Bosses face left, towards the pet. The pen origin is the centre of their feet.

static void boss_chispapo(const DvPen* p, uint32_t frame) {
    int32_t puff = (int32_t)(frame % 8 < 4);
    pen_ink(p, false);
    pen_ell(p, -18, -3, 8, 3);
    pen_ell(p, 18, -3, 8, 3);
    pen_ell(p, 0, -16, 26, 14 + puff);
    pen_ell(p, -10, -30, 6, 6);
    pen_ell(p, 10, -30, 6, 6);
    pen_ink(p, true);
    pen_box(p, -11, -33, 2, 6);
    pen_box(p, 9, -33, 2, 6);
    if((frame / 5) % 3 == 0) {
        pen_ell(p, 0, -14, 18, 3);
    } else {
        pen_line(p, -16, -15, 16, -15);
    }
    for(int32_t y = -8; y < -2; y += 3)
        pen_line(p, -10, y, 10, y);
    pen_ink(p, false);
    if(p->sil) return;
    // Crackle: four short lightning forks off the body.
    for(uint32_t i = 0; i < 4; i++) {
        uint32_t h = dw_hash(frame * 4 + i);
        int32_t a = (int32_t)(h % 8);
        int32_t x = cos8[a] * 27 / 64;
        int32_t y = -16 + sin8[a] * 16 / 64;
        pen_line(p, x, y, x + cos8[a] / 12 + (int32_t)((h >> 8) % 5) - 2, y + sin8[a] / 16);
    }
}

static void boss_cableboa(const DvPen* p, uint32_t frame) {
    int32_t sway = (int32_t)(frame % 8 < 4) * 2;
    pen_ink(p, false);
    pen_ell(p, 0, -5, 26, 5);
    pen_ell(p, 2, -13, 21, 5);
    pen_ell(p, 4, -21, 17, 4);
    for(int32_t i = 0; i < 6; i++)
        pen_ell(p, 10 - i * 4 + (i % 2) * sway, -27 - i * 3, 4, 3);
    pen_box(p, -26 + sway, -48, 18, 11);
    pen_box(p, -31 + sway, -46, 6, 2);
    pen_box(p, -31 + sway, -41, 6, 2);
    pen_ink(p, true);
    for(int32_t x = -20; x < 22; x += 6) {
        pen_line(p, x, -8, x + 2, -2);
        pen_line(p, x + 2, -16, x + 4, -10);
    }
    pen_box(p, -23 + sway, -46, 12, 7);
    pen_ink(p, false);
    pen_ell(p, -17 + sway, -43, 2, 2);
    if(!p->sil && frame % 2) {
        pen_line(p, -32 + sway, -45, -36 + sway, -48);
        pen_line(p, -32 + sway, -40, -37 + sway, -37);
    }
}

static void boss_mastiram(const DvPen* p, uint32_t frame) {
    pen_ink(p, false);
    for(int32_t i = 0; i < 3; i++) {
        int32_t x = -6 + i * 10;
        int32_t top = -44 + i * 3;
        pen_line(p, x, top, x - 3, -22);
        pen_line(p, x, top, x + 3, -22);
        for(int32_t y = top + 4; y < -22; y += 5) {
            pen_line(p, x - 2, y, x + 2, y + 4);
            pen_line(p, x + 2, y, x - 2, y + 4);
        }
        if((frame + (uint32_t)i * 3) % 6 < 3) pen_ell(p, x, top - 2, 1, 1);
        if(!p->sil && (frame + (uint32_t)i) % 6 == 0) {
            digivice_draw_ring(p->canvas, pen_x(p, x), p->cy + top - 2, 4, 2);
        }
    }
    pen_ink(p, false);
    pen_ell(p, 2, -16, 22, 10);
    for(int32_t x = -14; x <= 16; x += 10)
        pen_box(p, x, -9, 4, 9);
    pen_ell(p, -20, -20, 8, 8);
    pen_ell(p, -27, -16, 5, 4);
    pen_ink(p, true);
    pen_ell(p, -18, -22, 5, 5);
    pen_ink(p, false);
    pen_ell(p, -18, -22, 3, 3);
    pen_ink(p, true);
    pen_ell(p, -18, -22, 1, 1);
    pen_dot(p, -26, -20);
    pen_ink(p, false);
}

static void boss_cristalok(const DvPen* p, uint32_t frame) {
    int32_t lift = (int32_t)(frame % 8 < 4);
    pen_ink(p, false);
    for(int32_t x = -16; x <= 16; x += 11) {
        pen_line(p, x, -10, x - 5, 0);
        pen_line(p, x + 1, -10, x - 4, 0);
    }
    pen_ell(p, 2, -16 - lift, 24, 12);
    pen_ell(p, -22, -14 - lift, 7, 6);
    pen_line(p, -28, -16 - lift, -33, -20 - lift);
    pen_line(p, -28, -12 - lift, -33, -8 - lift);
    pen_ink(p, true);
    // Facets.
    pen_line(p, -18, -18 - lift, -6, -26 - lift);
    pen_line(p, -6, -26 - lift, 8, -26 - lift);
    pen_line(p, 8, -26 - lift, 22, -18 - lift);
    pen_line(p, -6, -26 - lift, 2, -12 - lift);
    pen_line(p, 8, -26 - lift, 2, -12 - lift);
    pen_line(p, -18, -10 - lift, 22, -10 - lift);
    pen_ell(p, 2, -19 - lift, 4, 4);
    pen_box(p, -25, -16 - lift, 2, 2);
    pen_ink(p, false);
    int32_t a = (int32_t)(frame % 8);
    pen_line(p, 2, -19 - lift, 2 + cos8[a] * 3 / 64, -19 - lift + sin8[a] * 3 / 64);
}

static void boss_gusaferro(const DvPen* p, uint32_t frame) {
    pen_ink(p, false);
    int32_t hx = 0;
    int32_t hy = 0;
    for(int32_t i = 0; i < 8; i++) {
        int32_t wob = (int32_t)(((frame / 2 + (uint32_t)i) % 4 < 2) ? 2 : -2);
        int32_t x = 12 - i * 3 + wob;
        int32_t y = -6 - i * 5;
        int32_t r = 10 - i;
        pen_ell(p, x, y, r, r * 4 / 5);
        if(!p->sil) {
            pen_line(p, x - r - 2, y, x - r, y);
            pen_line(p, x + r, y - 1, x + r + 2, y - 2);
        }
        hx = x;
        hy = y;
    }
    pen_ell(p, hx - 6, hy - 2, 8, 8);
    pen_ink(p, true);
    pen_ell(p, hx - 8, hy - 2, 5, 6);
    pen_ink(p, false);
    for(int32_t t = 0; t < 8; t += 2) {
        int32_t a = (int32_t)((frame + (uint32_t)t) % 8);
        pen_tri(
            p,
            hx - 8 + cos8[a] * 5 / 64,
            hy - 2 + sin8[a] * 6 / 64,
            hx - 8 + cos8[(a + 1) % 8] * 5 / 64,
            hy - 2 + sin8[(a + 1) % 8] * 6 / 64,
            hx - 8,
            hy - 2);
    }
    if(!p->sil) {
        for(uint32_t i = 0; i < 8; i++) {
            uint32_t h = dw_hash(frame / 2 * 8 + i);
            pen_dot(p, -24 + (int32_t)(h % 48), -(int32_t)((h >> 8) % 3));
        }
    }
}

static void boss_centinela(const DvPen* p, uint32_t frame) {
    bool on = (frame / 3) % 2 == 0;
    pen_ink(p, false);
    pen_tri(p, -20, 0, 20, 0, 0, -34);
    pen_box(p, -8, -36, 16, 6);
    pen_ell(p, 0, -41, 7, 7);
    for(int32_t sd = -1; sd <= 1; sd += 2) {
        int32_t sway = (int32_t)(frame % 8 < 4);
        pen_line(p, sd * 6, -26, sd * 22, -22 + sway);
        pen_line(p, sd * 6, -25, sd * 22, -21 + sway);
        pen_line(p, sd * 22, -22 + sway, sd * 26, -27 + sway);
        pen_line(p, sd * 22, -22 + sway, sd * 27, -18 + sway);
    }
    pen_ink(p, true);
    for(int32_t y = -26; y < 0; y += 6) {
        int32_t w = (y + 34) * 20 / 34;
        pen_line(p, -w + 3, y, w - 3, y);
        pen_line(p, -w + 3, y, w - 6, y + 6);
    }
    if(on) {
        pen_ell(p, 0, -41, 5, 5);
        pen_ink(p, false);
        pen_ell(p, 0, -41, 2, 2);
        if(!p->sil) {
            for(int32_t i = 0; i < 8; i++) {
                pen_line(
                    p,
                    cos8[i] * 10 / 64,
                    -41 + sin8[i] * 9 / 64,
                    cos8[i] * 14 / 64,
                    -41 + sin8[i] * 12 / 64);
            }
        }
    } else {
        pen_ell(p, 0, -41, 5, 5);
        pen_ink(p, false);
        pen_ell(p, -2, -41, 2, 2);
    }
    pen_ink(p, false);
}

static void boss_desborde(const DvPen* p, uint32_t frame) {
    int32_t o = (int32_t)(frame % 8 < 4) * 2;
    pen_ink(p, false);
    for(int32_t i = 0; i < 4; i++)
        pen_tri(p, -6 + i * 8, -32 + o, 2 + i * 8, -32 + o, i * 8, -44 + o + i * 2);
    pen_ell(p, 4, -20 + o, 26, 13);
    pen_tri(p, 26, -22 + o, 30, -36 + o, 34, -14 + o);
    pen_ell(p, -20, -18 + o, 9, 7);
    pen_ink(p, true);
    for(int32_t x = -27; x < -14; x += 3) {
        pen_tri(p, x, -22 + o, x + 2, -22 + o, x + 1, -19 + o);
        pen_tri(p, x, -14 + o, x + 2, -14 + o, x + 1, -17 + o);
    }
    pen_ell(p, -8, -26 + o, 3, 2);
    pen_line(p, 6, -16 + o, 20, -16 + o);
    pen_line(p, 4, -12 + o, 22, -12 + o);
    pen_ink(p, false);
    pen_dot(p, -8, -26 + o);
    // The wave it rides; bits spill from its mouth.
    for(int32_t x = -28; x <= 28; x += 2)
        pen_dot(p, x, -1 - (int32_t)(((uint32_t)(x + 28) / 2 + frame) % 3));
    if(!p->sil) {
        for(int32_t i = 0; i < 3; i++) {
            int32_t y = -16 + (int32_t)((frame * 2 + (uint32_t)i * 5) % 14);
            pen_dot(p, -26 - i * 2, y);
        }
    }
}

static void boss_negativo(
    Canvas* canvas,
    DigiviceForm pet,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    bool sil) {
    // The player's own pet in negative: its silhouette punched out of a black field.
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_form(canvas, pet, cx, foot_y - 22, frame, DigivicePoseAttack, true);
    if(sil) return;
    digivice_draw_invert(canvas, cx - 24, foot_y - 46, 48, 46);
    canvas_set_color(canvas, ColorWhite);
    for(uint32_t i = 0; i < 10; i++) {
        uint32_t h = dw_hash(frame / 2 * 10 + i);
        int32_t dx = (int32_t)(h % 44);
        int32_t dy = (int32_t)((h >> 8) % 42);
        digivice_draw_dot(canvas, cx - 22 + dx, foot_y - 44 + dy);
    }
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_boss(
    Canvas* canvas,
    uint8_t area,
    DigiviceForm pet,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    bool silhouette) {
    DvPen p = {.canvas = canvas, .cx = cx, .cy = foot_y, .flip = false, .sil = silhouette};
    switch(area) {
    case 0:
        boss_chispapo(&p, frame);
        break;
    case 1:
        boss_cableboa(&p, frame);
        break;
    case 2:
        boss_mastiram(&p, frame);
        break;
    case 3:
        boss_cristalok(&p, frame);
        break;
    case 4:
        boss_gusaferro(&p, frame);
        break;
    case 5:
        boss_centinela(&p, frame);
        break;
    case 6:
        boss_desborde(&p, frame);
        break;
    default:
        boss_negativo(canvas, pet, cx, foot_y, frame, silhouette);
        break;
    }
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_shot(
    Canvas* canvas,
    DigiviceAttr attr,
    int32_t x,
    int32_t y,
    uint8_t size,
    int8_t dir,
    uint32_t frame) {
    canvas_set_color(canvas, ColorBlack);
    if(size == DIGIVICE_SHOT_BEAM) {
        int32_t x0 = dir > 0 ? x - 8 : x;
        digivice_draw_box(canvas, x0, y - 2, 9, 5);
        canvas_set_color(canvas, ColorWhite);
        digivice_draw_box(canvas, x0 + 1, y - 1, 7, 3);
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_box(canvas, x - dir * 4, y, 2, 1);
        for(int32_t i = 1; i < 4; i++) {
            int32_t h = 5 - 2 * i;
            digivice_draw_box(canvas, x - dir * (9 + i * 4), y - h / 2, 3, h < 1 ? 1 : h);
        }
        return;
    }
    if(size <= 2) {
        // The Baby's 2x2 pellet with one trail dot.
        digivice_draw_box(canvas, x - 1, y - 1, 2, 2);
        if(frame % 2) digivice_draw_dot(canvas, x - dir * 3, y);
        return;
    }
    int32_t r = size / 2;
    switch(attr) {
    case DigiviceAttrPortador:
        // Three concentric arcs: a ring wave.
        for(int32_t k = 0; k < 3; k++) {
            int32_t ax = x - dir * k * 3;
            int32_t h = r + k;
            digivice_draw_line(canvas, ax - dir, y - h, ax, y - h / 2);
            digivice_draw_line(canvas, ax, y - h / 2, ax, y + h / 2);
            digivice_draw_line(canvas, ax, y + h / 2, ax - dir, y + h);
        }
        break;
    case DigiviceAttrRuido: {
        // Jagged two-pixel bolt.
        int32_t px = x - dir * r * 3;
        int32_t py = y;
        for(int32_t i = 0; i < 4; i++) {
            int32_t nx = px + dir * (r * 3 / 2 + 1);
            int32_t ny = y + (i % 2 ? r : -r);
            digivice_draw_line(canvas, px, py, nx, ny);
            digivice_draw_line(canvas, px, py + 1, nx, ny + 1);
            px = nx;
            py = ny;
        }
        break;
    }
    case DigiviceAttrCifrado:
        // Hollow packet with a blinking trail.
        digivice_draw_box(canvas, x - r, y - r, r * 2 + 1, r * 2 + 1);
        if(r > 1) {
            canvas_set_color(canvas, ColorWhite);
            digivice_draw_box(canvas, x - r + 1, y - r + 1, r * 2 - 1, r * 2 - 1);
            canvas_set_color(canvas, ColorBlack);
        }
        digivice_draw_dot(canvas, x, y);
        for(int32_t i = 1; i < 4; i++) {
            if((frame + (uint32_t)i) % 2) {
                digivice_draw_dot(canvas, x - dir * (r + i * 2), y + (i % 2 ? r : -r));
            }
        }
        break;
    default:
        // Nulo: a hollow circle.
        digivice_draw_ring(canvas, x, y, r + 1, r + 1);
        break;
    }
}

void digivice_draw_attr_icon(Canvas* canvas, DigiviceAttr attr, int32_t x, int32_t y) {
    canvas_set_color(canvas, ColorBlack);
    switch(attr) {
    case DigiviceAttrPortador:
        digivice_draw_ring(canvas, x + 3, y + 3, 3, 3);
        digivice_draw_dot(canvas, x + 1, y + 3);
        digivice_draw_dot(canvas, x + 2, y + 2);
        digivice_draw_dot(canvas, x + 3, y + 3);
        digivice_draw_dot(canvas, x + 4, y + 4);
        digivice_draw_dot(canvas, x + 5, y + 3);
        break;
    case DigiviceAttrRuido:
        digivice_draw_line(canvas, x + 5, y, x + 1, y + 3);
        digivice_draw_line(canvas, x + 1, y + 3, x + 5, y + 3);
        digivice_draw_line(canvas, x + 5, y + 3, x + 1, y + 6);
        digivice_draw_dot(canvas, x + 6, y);
        digivice_draw_dot(canvas, x, y + 6);
        break;
    case DigiviceAttrCifrado:
        canvas_draw_frame(canvas, x, y, 7, 7);
        digivice_draw_box(canvas, x + 2, y + 2, 3, 2);
        digivice_draw_box(canvas, x + 3, y + 4, 1, 2);
        break;
    default:
        digivice_draw_ring(canvas, x + 3, y + 3, 3, 3);
        digivice_draw_line(canvas, x + 1, y + 5, x + 5, y + 1);
        break;
    }
}

void digivice_draw_burst(Canvas* canvas, int32_t cx, int32_t cy, int32_t r) {
    for(int32_t i = 0; i < 8; i++) {
        digivice_draw_line(
            canvas,
            cx + cos8[i] * r * 45 / 6400,
            cy + sin8[i] * r * 45 / 6400,
            cx + cos8[i] * r / 64,
            cy + sin8[i] * r / 64);
    }
}

void digivice_draw_star5(Canvas* canvas, int32_t cx, int32_t cy) {
    digivice_draw_box(canvas, cx - 2, cy, 5, 1);
    digivice_draw_box(canvas, cx, cy - 2, 1, 5);
    digivice_draw_dot(canvas, cx - 1, cy - 1);
    digivice_draw_dot(canvas, cx + 1, cy - 1);
    digivice_draw_dot(canvas, cx - 1, cy + 1);
    digivice_draw_dot(canvas, cx + 1, cy + 1);
}

void digivice_draw_shield(Canvas* canvas, int32_t x, int32_t y) {
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_box(canvas, x, y, 5, 9);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_line(canvas, x, y, x + 4, y);
    digivice_draw_line(canvas, x, y, x, y + 7);
    digivice_draw_line(canvas, x + 4, y, x + 4, y + 7);
    digivice_draw_line(canvas, x + 1, y + 8, x + 3, y + 8);
    digivice_draw_box(canvas, x + 2, y + 2, 1, 5);
    digivice_draw_box(canvas, x + 1, y + 3, 3, 1);
}

void digivice_draw_exclam_bubble(Canvas* canvas, int32_t x, int32_t y) {
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_box(canvas, x, y, 9, 11);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_line(canvas, x + 1, y, x + 7, y);
    digivice_draw_line(canvas, x + 1, y + 10, x + 7, y + 10);
    digivice_draw_line(canvas, x, y + 1, x, y + 9);
    digivice_draw_line(canvas, x + 8, y + 1, x + 8, y + 9);
    // Tail towards the speaker, bottom left, hanging off the bottom border.
    digivice_draw_dot(canvas, x + 3, y + 11);
    digivice_draw_dot(canvas, x + 2, y + 12);
    digivice_draw_box(canvas, x + 4, y + 2, 1, 5);
    digivice_draw_dot(canvas, x + 4, y + 8);
}

void digivice_draw_bandage(Canvas* canvas, int32_t x, int32_t y) {
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_box(canvas, x - 1, y + 1, 9, 5);
    digivice_draw_box(canvas, x + 1, y - 1, 5, 9);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_box(canvas, x, y + 2, 7, 3);
    digivice_draw_box(canvas, x + 2, y, 3, 7);
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_dot(canvas, x + 3, y + 3);
    digivice_draw_dot(canvas, x + 1, y + 3);
    digivice_draw_dot(canvas, x + 5, y + 3);
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_invert(Canvas* canvas, int32_t x, int32_t y, int32_t w, int32_t h) {
    canvas_set_color(canvas, ColorXOR);
    digivice_draw_box(canvas, x, y, w, h);
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_charge_meter(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    uint8_t zone_start,
    uint8_t zone_w,
    uint8_t cursor) {
    canvas_set_color(canvas, ColorBlack);
    for(uint8_t i = 0; i < DIGIVICE_METER_SEGMENTS; i++) {
        int32_t sx = x + i * 5;
        if(i >= zone_start && i < zone_start + zone_w) {
            canvas_draw_frame(canvas, sx, y, 4, 5);
        } else {
            digivice_draw_box(canvas, sx, y + 3, 4, 2);
        }
    }
    digivice_draw_box(canvas, x + cursor * 5, y - 2, 4, 8);
}

void digivice_draw_ok_button(Canvas* canvas, int32_t x, int32_t y, bool down) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_fill_ellipse(canvas, x + 4, y + 4, 4, 4);
    if(!down) {
        canvas_set_color(canvas, ColorWhite);
        digivice_draw_fill_ellipse(canvas, x + 4, y + 3, 3, 3);
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_text(canvas, x + 1, y + 1, "OK");
    } else {
        canvas_set_color(canvas, ColorWhite);
        digivice_draw_text(canvas, x + 1, y + 2, "OK");
        canvas_set_color(canvas, ColorBlack);
    }
}

void digivice_draw_arrow(Canvas* canvas, int32_t x, int32_t y, bool up, bool on) {
    canvas_set_color(canvas, ColorBlack);
    if(up) {
        digivice_draw_fill_triangle(canvas, x, y + 4, x + 6, y + 4, x + 3, y);
    } else {
        digivice_draw_fill_triangle(canvas, x, y, x + 6, y, x + 3, y + 4);
    }
    if(on) return;
    canvas_set_color(canvas, ColorWhite);
    if(up) {
        digivice_draw_line(canvas, x + 2, y + 3, x + 4, y + 3);
        digivice_draw_dot(canvas, x + 3, y + 2);
    } else {
        digivice_draw_line(canvas, x + 2, y + 1, x + 4, y + 1);
        digivice_draw_dot(canvas, x + 3, y + 2);
    }
    canvas_set_color(canvas, ColorBlack);
}

void digivice_draw_hazard(Canvas* canvas, int32_t y, uint32_t frame) {
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_box(canvas, 0, y, 128, 7);
    canvas_set_color(canvas, ColorBlack);
    int32_t shift = (int32_t)((frame * 3) % 8);
    for(int32_t x = -8; x < 136; x += 8) {
        int32_t sx = x + shift;
        digivice_draw_fill_triangle(canvas, sx, y, sx + 4, y, sx - 3, y + 6);
        digivice_draw_fill_triangle(canvas, sx + 4, y, sx + 1, y + 6, sx - 3, y + 6);
    }
    digivice_draw_line(canvas, 0, y, 127, y);
    digivice_draw_line(canvas, 0, y + 6, 127, y + 6);
}
