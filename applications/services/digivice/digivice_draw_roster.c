#include "digivice_pen.h"
#include "digivice_config.h"
#include "digivice_roster.h"

typedef void (*MonDraw)(const DvPen* p, uint32_t frame, DigivicePose pose);

// Baby I, line A: a drop with a shine that only has eyes and bounces.
static void mon_pipo(const DvPen* p, uint32_t frame, DigivicePose pose) {
    DvPen q = *p;
    if(pose == DigivicePoseIdle && (frame / 3) % 2) q.cy -= 2;
    pen_ink(&q, false);
    pen_ell(&q, 0, 3, 7, 6);
    pen_tri(&q, -3, -2, 0, -9, 3, -2);
    pen_eye(&q, -4, 0, pose);
    pen_eye(&q, 2, 0, pose);
    pen_ink(&q, true);
    pen_dot(&q, -4, 5);
    pen_dot(&q, -3, 5);
    pen_ink(&q, false);
}

// Baby I, line B: a small ball with a spark antenna and a fringe.
static void mon_chispi(const DvPen* p, uint32_t frame, DigivicePose pose) {
    pen_ink(p, false);
    pen_ell(p, 0, 2, 6, 5);
    pen_line(p, 1, -3, 4, -8);
    pen_ell(p, 4, -9, 1, 1);
    if(pose == DigivicePoseIdle && (frame / 2) % 2) {
        pen_dot(p, 4, -13);
        pen_dot(p, 8, -9);
        pen_dot(p, 0, -11);
    }
    pen_ink(p, true);
    for(int32_t x = -4; x <= 4; x += 2) pen_dot(p, x, 7);
    pen_eye(p, -4, 1, pose);
    pen_eye(p, 2, 1, pose);
    pen_ink(p, true);
    pen_line(p, -1, 5, 0, 5);
    pen_ink(p, false);
}

// Baby II: a ball with long ears that sway.
static void mon_orejin(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t s = (int32_t)((frame / 3) % 2);
    pen_ink(p, false);
    pen_tri(p, -7, -3, -3, -5, -10, -14 + s * 2);
    pen_tri(p, 3, -5, 7, -3, 11, -14 + s * 2);
    pen_ell(p, 0, 2, 8, 7);
    pen_ell(p, -4, 8, 2, 1);
    pen_ell(p, 4, 8, 2, 1);
    pen_ink(p, true);
    pen_line(p, -7, -4, -9, -11 + s * 2);
    pen_line(p, 7, -4, 10, -11 + s * 2);
    pen_eye(p, -5, 0, pose);
    pen_eye(p, 3, 0, pose);
    pen_ink(p, true);
    pen_line(p, -1, 5, 1, 5);
    pen_ink(p, false);
}

// Baby II: a ball with a little horn and a curled tail that wags.
static void mon_colix(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t w = (int32_t)((frame / 3) % 3) - 1;
    pen_ink(p, false);
    pen_ell(p, 0, 1, 7, 6);
    pen_tri(p, 2, -4, 6, -4, 6, -10);
    pen_ell(p, -8, 3, 2, 2);
    pen_ell(p, -11, 2, 2, 2);
    pen_ell(p, -13, 0, 1, 1);
    pen_ell(p, -13 + w, -3, 1, 1);
    pen_ell(p, -12 + w * 2, -6, 2, 2);
    pen_eye(p, -3, -1, pose);
    pen_eye(p, 2, -1, pose);
    pen_ink(p, true);
    pen_line(p, -1, 4, 1, 4);
    pen_dot(p, -12 + w * 2, -6);
    pen_ink(p, false);
}

// Failure form: a sad puddle with flat ears and flies.
static void mon_charco(const DvPen* p, uint32_t frame, DigivicePose pose) {
    pen_ink(p, false);
    pen_ell(p, 0, 8, 12, 4);
    pen_ell(p, 0, 2, 7, 6);
    pen_tri(p, -6, 0, -12, 8, -5, 5);
    pen_tri(p, 6, 0, 12, 8, 5, 5);
    pen_eye(p, -4, 1, pose);
    pen_eye(p, 2, 1, pose);
    pen_ink(p, true);
    pen_line(p, -2, 6, 1, 5);
    pen_ink(p, false);
    pen_dot(p, 10 + (int32_t)((frame / 2) % 4), -8 + (int32_t)((frame / 3) % 3));
    pen_dot(p, -9 - (int32_t)((frame / 3) % 3), -6 + (int32_t)((frame / 2) % 4));
}

static const MonDraw mon_table[DigiviceFormCount] = {
    [DigiviceFormPipo] = mon_pipo,
    [DigiviceFormChispi] = mon_chispi,
    [DigiviceFormOrejin] = mon_orejin,
    [DigiviceFormColix] = mon_colix,
    [DigiviceFormCharco] = mon_charco,
};

static DigiviceFoe foe_of(DigiviceForm form, bool* known) {
    *known = true;
    switch(form) {
    case DigiviceFormKorno:
        return DigiviceFoeKorno;
    case DigiviceFormChipko:
        return DigiviceFoeChipko;
    case DigiviceFormEstatik:
        return DigiviceFoeEstatik;
    case DigiviceFormMokobit:
        return DigiviceFoeMokobit;
    case DigiviceFormFuzzcan:
        return DigiviceFoeFuzzcan;
    case DigiviceFormVampixel:
        return DigiviceFoeVampixel;
    case DigiviceFormSilbapo:
        return DigiviceFoeSilbapo;
    case DigiviceFormChatarro:
        return DigiviceFoeChatarro;
    default:
        *known = false;
        return DigiviceFoeCount;
    }
}

bool digivice_draw_roster(
    Canvas* canvas,
    DigiviceForm form,
    int32_t cx,
    int32_t cy,
    uint32_t frame,
    DigivicePose pose,
    bool silhouette) {
    if(form >= DigiviceFormCount) return false;
    bool is_foe;
    DigiviceFoe foe = foe_of(form, &is_foe);
    if(is_foe) {
        // The trail monsters stand on a foot line: 12 px below the centre (16 for Champions).
        int32_t foot = cy + (digivice_expedition_foe_champion(foe) ? 16 : 12);
        digivice_draw_foe(canvas, foe, cx, foot, frame, pose, false, silhouette);
        return true;
    }
    MonDraw draw = mon_table[form];
    if(!draw) return false;
    DvPen p = {.canvas = canvas, .cx = cx, .cy = cy, .flip = false, .sil = silhouette};
    draw(&p, frame, pose);
    canvas_set_color(canvas, ColorBlack);
    return true;
}
