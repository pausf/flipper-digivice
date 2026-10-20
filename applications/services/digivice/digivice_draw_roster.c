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

// Champion, Portador: a woolly ram with curled horns and a wave on the flank.
static void mon_ariete(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_ell(p, -2, 3 + bob, 13, 9);
    pen_ell(p, 10, -4 + bob, 6, 6);
    pen_ell(p, 12, -10 + bob, 3, 3);
    pen_ell(p, 6, -9 + bob, 3, 3);
    pen_box(p, -10, 11, 3, 6);
    pen_box(p, 4, 11, 3, 6);
    pen_eye(p, 10, -6 + bob, pose);
    pen_ink(p, true);
    pen_line(p, -6, 2 + bob, -3, 0 + bob);
    pen_line(p, -3, 0 + bob, 0, 2 + bob);
    pen_line(p, 0, 2 + bob, 3, 0 + bob);
    pen_dot(p, 13, -10 + bob);
    pen_ink(p, false);
}

// Champion, Portador: a slim wolf with pointed ears, a flame tail and a wave on the flank.
static void mon_ecolobo(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_tri(p, -10, 2 + bob, -9, 7 + bob, -20, -8 + bob * 3);
    pen_ell(p, -3, 4 + bob, 11, 5);
    pen_ell(p, 6, 2 + bob, 6, 7);
    pen_box(p, -12, 8, 3, 9);
    pen_box(p, -7, 8, 3, 9);
    pen_box(p, 4, 8, 3, 9);
    pen_box(p, 9, 8, 3, 9);
    pen_ell(p, 11, -6 + bob, 6, 5);
    pen_tri(p, 13, -9 + bob, 20, -4 + bob, 13, -1 + bob);
    pen_tri(p, 6, -10 + bob, 10, -11 + bob, 7, -18 + bob);
    pen_tri(p, 11, -11 + bob, 15, -10 + bob, 15, -17 + bob);
    pen_eye(p, 12, -8 + bob, pose);
    pen_ink(p, true);
    for(int32_t x = -11; x < 3; x++)
        pen_dot(p, x, 4 + bob + ((x + (int32_t)frame / 3) % 4 < 2 ? 0 : 1));
    pen_line(p, -17, -4 + bob * 3, -12, 1 + bob);
    pen_ink(p, false);
}

// Champion, Ruido: a bat whose dark wings glitch one column sideways.
static void mon_vampixel(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 3) % 2 ? 1 : 0;
    int32_t gl = (int32_t)((frame / 2) % 2);
    pen_ink(p, false);
    for(int32_t sd = -1; sd <= 1; sd += 2) {
        pen_tri(p, sd * 4, -2 + bob, sd * 20, -14 + bob, sd * 18, 2 + bob);
        pen_tri(p, sd * 4, -2 + bob, sd * 18, 2 + bob, sd * 13, 10 + bob);
        pen_tri(p, sd * 4, 2 + bob, sd * 13, 10 + bob, sd * 7, 12 + bob);
    }
    pen_ell(p, 0, 5 + bob, 6, 8);
    pen_ell(p, 0, -6 + bob, 6, 5);
    pen_tri(p, -7, -8 + bob, -3, -11 + bob, -8, -18 + bob);
    pen_tri(p, 3, -11 + bob, 7, -8 + bob, 8, -18 + bob);
    pen_box(p, -4, 12, 2, 6);
    pen_box(p, 2, 12, 2, 6);
    pen_eye(p, -4, -8 + bob, pose);
    pen_eye(p, 2, -8 + bob, pose);
    pen_ink(p, true);
    pen_line(p, -3, -2 + bob, -1, -2 + bob);
    pen_dot(p, -2, -1 + bob);
    pen_dot(p, 2, -1 + bob);
    // Glitch: one wing column is shifted sideways and the gap is left blank.
    pen_line(p, 12, -2 + bob, 12, 8 + bob);
    pen_ink(p, false);
    pen_line(p, 12 + gl * 2, -2 + bob, 12 + gl * 2, 6 + bob);
    pen_dot(p, -12 - gl, -6 + bob);
}

// Champion, Cifrado: a beetle with a padlock shell and a key horn.
static void mon_cerrojo(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_box(p, -17, -6 + bob, 26, 17 - bob);
    pen_box(p, 7, 2, 10, 9);
    pen_tri(p, 13, 3, 18, 3, 20, -12);
    pen_tri(p, 16, -6, 20, -9, 20, -3);
    pen_box(p, -14, 11, 3, 7);
    pen_box(p, -6, 11, 3, 7);
    pen_box(p, 2, 11, 3, 7);
    pen_eye(p, 10, 4, pose);
    pen_ink(p, true);
    pen_ell(p, -5, 0 + bob, 3, 3);
    pen_tri(p, -7, 1 + bob, -3, 1 + bob, -5, 6 + bob);
    pen_dot(p, -14, -4 + bob);
    pen_dot(p, 4, -4 + bob);
    pen_dot(p, -14, 8 + bob);
    pen_dot(p, 4, 8 + bob);
    pen_ink(p, false);
}

// Champion, Cifrado: an owl with a scanner visor and a grid of feathers.
static void mon_buhoscan(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_tri(p, -11, -9 + bob, -5, -11 + bob, -13, -18 + bob);
    pen_tri(p, 5, -11 + bob, 11, -9 + bob, 13, -18 + bob);
    pen_ell(p, 0, 2 + bob, 11, 13 - bob);
    pen_ell(p, -12, 4 + bob, 4, 8);
    pen_ell(p, 12, 4 + bob, 4, 8);
    pen_box(p, -5, 14, 3, 4);
    pen_box(p, 2, 14, 3, 4);
    pen_ink(p, true);
    pen_box(p, -10, -8 + bob, 20, 8);
    pen_ink(p, false);
    pen_eye(p, -7, -6 + bob, pose);
    pen_eye(p, 4, -6 + bob, pose);
    int32_t sy = -7 + bob + (int32_t)((frame / 2) % 6);
    pen_line(p, -10, sy, 9, sy);
    pen_tri(p, -2, 1 + bob, 2, 1 + bob, 0, 5 + bob);
    pen_ink(p, true);
    for(int32_t y = 6; y < 13; y += 2)
        for(int32_t x = -5 + (y % 4 ? 1 : 0); x < 6; x += 3) pen_dot(p, x, y + bob);
    pen_ink(p, false);
}

static const MonDraw mon_table[DigiviceFormCount] = {
    [DigiviceFormPipo] = mon_pipo,
    [DigiviceFormChispi] = mon_chispi,
    [DigiviceFormOrejin] = mon_orejin,
    [DigiviceFormColix] = mon_colix,
    [DigiviceFormCharco] = mon_charco,
    [DigiviceFormAriete] = mon_ariete,
    [DigiviceFormEcolobo] = mon_ecolobo,
    [DigiviceFormVampixel] = mon_vampixel,
    [DigiviceFormCerrojo] = mon_cerrojo,
    [DigiviceFormBuhoscan] = mon_buhoscan,
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
