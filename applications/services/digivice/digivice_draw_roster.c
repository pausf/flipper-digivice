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

// Ultimate, Portador: a knight with an antenna lance and a wave shield.
static void mon_lanzero(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_line(p, 10, 8, 20, -19);
    pen_line(p, 11, 8, 21, -19);
    pen_tri(p, 18, -17, 22, -17, 21, -23);
    pen_box(p, -7, 8, 5, 12);
    pen_box(p, 1, 8, 5, 12);
    pen_box(p, -9, 19, 8, 3);
    pen_box(p, 1, 19, 8, 3);
    pen_box(p, -7, -8 + bob, 14, 16 - bob);
    pen_box(p, 8, -4 + bob, 4, 9);
    pen_ell(p, 0, -13 + bob, 6, 6);
    pen_line(p, 0, -19 + bob, 2, -22 + bob);
    pen_ell(p, -12, 2 + bob, 6, 9);
    pen_ink(p, true);
    pen_box(p, -5, -15 + bob, 11, 4);
    pen_ink(p, false);
    pen_eye(p, 1, -15 + bob, pose);
    pen_ink(p, true);
    pen_box(p, -5, -15 + bob, 4, 2);
    for(int32_t x = -16; x <= -8; x++)
        pen_dot(p, x, 2 + bob + (((x + (int32_t)frame / 2) % 4) < 2 ? -2 : 2));
    pen_ink(p, false);
    pen_dot(p, 2, -22 + bob);
}

// Ultimate, Ruido: a long-necked wyvern whose wings crackle.
static void mon_wyvolt(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    int32_t j = (int32_t)(frame % 2);
    pen_ink(p, false);
    pen_tri(p, -4, 0, -24, -14 + j * 2, -8, -22 + j * 2);
    pen_tri(p, -4, 2, -24, -4 + j * 2, -10, 10);
    pen_tri(p, -8, 12, -10, 18, -24, 22);
    pen_ell(p, -2, 8 + bob, 9, 8);
    pen_ell(p, 5, 0 + bob, 4, 4);
    pen_ell(p, 9, -7 + bob, 3, 4);
    pen_ell(p, 13, -13 + bob, 6, 4);
    pen_tri(p, 16, -15 + bob, 26, -10 + bob, 16, -9 + bob);
    pen_tri(p, 8, -16 + bob, 12, -17 + bob, 6, -22 + bob);
    pen_box(p, -8, 14, 4, 8);
    pen_box(p, 2, 14, 4, 8);
    pen_eye(p, 12, -15 + bob, pose);
    pen_ink(p, true);
    pen_line(p, -3, 3, -20, -12 + j * 2);
    pen_line(p, -3, 4, -20, -3 + j * 2);
    for(int32_t y = 7; y < 16; y += 3) pen_line(p, -2, y + bob, 4, y + bob);
    pen_ink(p, false);
    if(j) {
        pen_line(p, -24, -6, -21, -9);
        pen_line(p, -21, -9, -18, -5);
        pen_line(p, -18, -5, -15, -8);
    }
}

// Ultimate, Ruido: a crowned mud king with tentacle arms and a falling drip.
static void mon_fangorey(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_ell(p, 0, 17, 22, 4);
    for(int32_t sd = -1; sd <= 1; sd += 2)
        for(int32_t i = 0; i < 4; i++) {
            int32_t y = 4 + i * 3 + (int32_t)((frame / 3 + (uint32_t)i) % 2);
            pen_ell(p, sd * (12 + i * 3), y, 3 - i / 2, 3 - i / 2);
        }
    pen_ell(p, 0, 4 + bob, 13, 11 - bob);
    pen_box(p, -8, -12 + bob, 17, 4);
    for(int32_t i = 0; i < 3; i++) {
        int32_t x = -8 + i * 6;
        pen_tri(p, x, -12 + bob, x + 4, -12 + bob, x + 2, -19 + bob);
    }
    pen_eye(p, -6, -1 + bob, pose);
    pen_eye(p, 4, -1 + bob, pose);
    pen_ink(p, true);
    pen_line(p, -8, 10 + bob, 8, 10 + bob);
    for(int32_t x = -4; x <= 4; x += 2) pen_dot(p, x, 6 + bob + (x & 2 ? 0 : 1));
    pen_ink(p, false);
    pen_dot(p, -14, 11 + (int32_t)(frame % 6));
    pen_dot(p, 12, 11 + (int32_t)((frame + 3) % 6));
}

// Ultimate, Cifrado: a vault titan with a safe door and a turning dial.
static void mon_bovedax(const DvPen* p, uint32_t frame, DigivicePose pose) {
    static const int8_t hand[8][2] = {
        {0, -4}, {3, -3}, {4, 0}, {3, 3}, {0, 4}, {-3, 3}, {-4, 0}, {-3, -3}};
    uint32_t k = (frame / 2) % 8;
    pen_ink(p, false);
    pen_box(p, -12, 8, 7, 12);
    pen_box(p, 5, 8, 7, 12);
    pen_box(p, -21, -9, 6, 20);
    pen_box(p, 15, -9, 6, 20);
    pen_box(p, -22, 10, 8, 6);
    pen_box(p, 14, 10, 8, 6);
    pen_box(p, -14, -10, 28, 20);
    pen_box(p, -6, -19, 12, 9);
    pen_ink(p, true);
    pen_ell(p, 0, 0, 8, 8);
    pen_ink(p, false);
    pen_ell(p, 0, 0, 7, 7);
    pen_ink(p, true);
    pen_ell(p, 0, 0, 5, 5);
    pen_ink(p, false);
    pen_line(p, 0, 0, hand[k][0], hand[k][1]);
    pen_dot(p, 0, 0);
    pen_ink(p, true);
    pen_dot(p, -12, -8);
    pen_dot(p, 11, -8);
    pen_dot(p, -12, 8);
    pen_dot(p, 11, 8);
    pen_ink(p, false);
    pen_eye(p, -4, -17, pose);
    pen_eye(p, 1, -17, pose);
}

// Ultimate, Nulo: a scrap colossus drawn in outline, with a hollow V torso and one eye.
static void mon_koloso(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t sway = pose == DigivicePoseIdle && (frame / 5) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_box(p, -14, -12 + sway, 28, 4);
    pen_line(p, -14, -9 + sway, 0, 10);
    pen_line(p, 14, -9 + sway, 0, 10);
    pen_line(p, -13, -9 + sway, 0, 9);
    pen_line(p, 13, -9 + sway, 0, 9);
    pen_box(p, -20, -10 + sway, 5, 22);
    pen_box(p, 15, -10 + sway, 5, 14);
    pen_box(p, -6, 8, 4, 12);
    pen_box(p, 3, 8, 4, 12);
    pen_ell(p, 0, -18 + sway, 5, 4);
    pen_eye(p, -1, -19 + sway, pose);
}

// Mega, Portador: a seraph with four sine-wave wings and a halo.
static void mon_aureola(const DvPen* p, uint32_t frame, DigivicePose pose) {
    static const int8_t wave[6] = {0, 1, 2, 1, 0, -1};
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    uint32_t k = (frame / 2) % 3;
    pen_ink(p, false);
    for(int32_t sd = -1; sd <= 1; sd += 2) {
        for(int32_t row = 0; row < 2; row++) {
            int32_t px = sd * 5;
            int32_t py = 0 + row * 8;
            for(int32_t i = 1; i <= 5; i++) {
                int32_t nx = sd * (5 + i * 4);
                int32_t ny = row * 8 - i * (row ? 1 : 3) + wave[((uint32_t)i + k * 2) % 6];
                pen_line(p, px, py + bob, nx, ny + bob);
                pen_line(p, px, py + bob + 1, nx, ny + bob + 1);
                px = nx;
                py = ny;
            }
        }
    }
    pen_tri(p, -10, 20, 10, 20, 0, -4);
    pen_ell(p, 0, -2 + bob, 5, 4);
    pen_ell(p, 0, -9 + bob, 5, 5);
    pen_box(p, -7, 20, 15, 2);
    pen_ell(p, 0, -18 + bob, 7, 2);
    pen_ink(p, true);
    pen_ell(p, 0, -18 + bob, 5, 1);
    pen_eye(p, -3, -10 + bob, pose);
    pen_eye(p, 2, -10 + bob, pose);
    pen_ink(p, false);
}

// Mega, Ruido: a static dragon with horns, jagged wings dithered by the frame, and plates.
static void mon_drakstat(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_tri(p, -4, -2, -24, -18, -22, 2);
    pen_tri(p, -4, 2, -22, 2, -12, 10);
    pen_ell(p, 0, 6 + bob, 10, 11);
    pen_ell(p, 6, -4 + bob, 5, 5);
    pen_ell(p, 11, -11 + bob, 6, 5);
    pen_box(p, 14, -12 + bob, 9, 6);
    pen_tri(p, 7, -15 + bob, 10, -16 + bob, 2, -22 + bob);
    pen_tri(p, 11, -16 + bob, 14, -15 + bob, 14, -22 + bob);
    pen_box(p, -8, 14, 5, 8);
    pen_box(p, 4, 14, 5, 8);
    pen_ink(p, true);
    for(int32_t x = -23; x < -4; x++)
        for(int32_t y = -18 + (x + 24) * 4 / 5; y <= 1; y++)
            if((x + y + (int32_t)frame) & 1) pen_dot(p, x, y);
    for(int32_t i = 0; i < 4; i++) {
        int32_t x = -6 + i * 3;
        pen_tri(p, x, -2 - i * 2, x + 3, -3 - i * 2, x, -7 - i * 2);
    }
    pen_ink(p, false);
    pen_eye(p, 10, -12 + bob, pose);
}

// Mega, Cifrado: a floating hooded oracle with a keyhole eye and three orbiting cubes.
static void mon_orakulo(const DvPen* p, uint32_t frame, DigivicePose pose) {
    static const int8_t orb[8][2] = {
        {14, 0}, {10, 4}, {0, 6}, {-10, 4}, {-14, 0}, {-10, -4}, {0, -6}, {10, -4}};
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    uint32_t k = (frame / 2) % 8;
    pen_ink(p, false);
    pen_tri(p, -17, 20 + bob, 17, 20 + bob, 0, -8);
    pen_box(p, -18, -6 + bob, 8, 12);
    pen_box(p, 10, -6 + bob, 8, 12);
    pen_tri(p, -11, -9 + bob, 11, -9 + bob, 0, -21 + bob);
    pen_box(p, -10, -12 + bob, 20, 7);
    pen_ink(p, true);
    pen_box(p, -7, -15 + bob, 14, 9);
    pen_ink(p, false);
    pen_eye(p, -5, -13 + bob, pose);
    pen_eye(p, 2, -13 + bob, pose);
    pen_ink(p, true);
    pen_ell(p, 0, 3 + bob, 4, 4);
    pen_ink(p, false);
    pen_ell(p, 0, 2 + bob, 1, 1);
    pen_box(p, 0, 3 + bob, 1, 3);
    for(uint32_t i = 0; i < 3; i++) {
        uint32_t o = (k + i * 3) % 8;
        pen_box(p, orb[o][0] - 1, orb[o][1] + 2 + bob, 2, 2);
    }
}

// Mega, Nulo: a sovereign whose cloak holds its body as negative space, under a hollow crown.
static void mon_nularca(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    int32_t wv = (int32_t)((frame / 3) % 2);
    pen_ink(p, false);
    pen_tri(p, -21, 22, 21, 22, 0, -10);
    pen_ell(p, -10, -3, 5, 4);
    pen_ell(p, 10, -3, 5, 4);
    pen_ink(p, true);
    for(int32_t x = -20; x < 21; x += 5) pen_box(p, x, 20 + wv, 2, 3);
    pen_ell(p, 0, 7 + bob, 7, 13);
    pen_ink(p, false);
    pen_ell(p, 0, 7 + bob, 5, 11);
    pen_ink(p, true);
    pen_ell(p, 0, 7 + bob, 3, 9);
    pen_ink(p, false);
    pen_ell(p, 0, -10 + bob, 5, 5);
    pen_eye(p, -3, -11 + bob, pose);
    pen_eye(p, 2, -11 + bob, pose);
    pen_line(p, -6, -17 + bob, 6, -17 + bob);
    pen_line(p, -6, -17 + bob, -6, -22 + bob);
    pen_line(p, 0, -17 + bob, 0, -22 + bob);
    pen_line(p, 6, -17 + bob, 6, -22 + bob);
    pen_dot(p, -22, 8 - wv * 3);
    pen_dot(p, 22, 4 - wv * 3);
}

// Special, Portador: a faceted crystal regent with waves above and a grid below.
static void mon_prismar(const DvPen* p, uint32_t frame, DigivicePose pose) {
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    pen_ink(p, false);
    pen_tri(p, 0, -20 + bob, 13, 0, -13, 0);
    pen_tri(p, -13, 0, 13, 0, 0, 20);
    pen_tri(p, -19, -10, -11, -2, -21, 5);
    pen_tri(p, 19, -10, 21, 5, 11, -2);
    pen_tri(p, -4, -18 + bob, 4, -18 + bob, 0, -23 + bob);
    pen_eye(p, -3, -8 + bob, pose);
    pen_eye(p, 2, -8 + bob, pose);
    pen_ink(p, true);
    pen_line(p, 0, 0, 0, 19);
    pen_line(p, -13, 0, 13, 0);
    pen_line(p, 0, 19, -7, 0);
    pen_line(p, 0, 19, 7, 0);
    for(int32_t x = -8; x < 9; x++)
        pen_dot(p, x, -3 + (((x + (int32_t)frame / 2) & 3) < 2 ? 0 : 1));
    for(int32_t y = 4; y < 16; y += 3)
        for(int32_t x = -4; x < 5; x += 3) pen_dot(p, x, y);
    pen_ink(p, false);
}

// Special, Nulo: a beacon ghost with a lantern on its head that sweeps two rays.
static void mon_baliza(const DvPen* p, uint32_t frame, DigivicePose pose) {
    static const int8_t ray[4][2] = {{14, -6}, {16, 0}, {14, 6}, {16, -3}};
    int32_t bob = pose == DigivicePoseIdle && (frame / 4) % 2 ? 1 : 0;
    uint32_t k = (frame / 3) % 4;
    pen_ink(p, false);
    pen_ell(p, 0, 4 + bob, 10, 11);
    pen_box(p, -10, 4 + bob, 21, 14);
    for(int32_t i = 0; i < 5; i++)
        pen_ell(p, -8 + i * 4, 18 + (((int32_t)frame / 3 + i) & 1), 2, 2);
    pen_ell(p, -13, 8 + bob, 3, 2);
    pen_ell(p, 13, 8 + bob, 3, 2);
    pen_box(p, -5, -17 + bob, 11, 8);
    pen_tri(p, -6, -17 + bob, 6, -17 + bob, 0, -22 + bob);
    pen_ink(p, true);
    pen_box(p, -3, -15 + bob, 7, 4);
    pen_ink(p, false);
    pen_line(p, 0, -17 + bob, 0, -9 + bob);
    pen_eye(p, -4, 2 + bob, pose);
    pen_eye(p, 3, 2 + bob, pose);
    pen_line(p, 6, -13 + bob, 6 + ray[k][0], -13 + bob + ray[k][1]);
    pen_line(p, -5, -13 + bob, -5 - ray[k][0], -13 + bob + ray[k][1]);
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
    [DigiviceFormLanzero] = mon_lanzero,
    [DigiviceFormWyvolt] = mon_wyvolt,
    [DigiviceFormFangorey] = mon_fangorey,
    [DigiviceFormBovedax] = mon_bovedax,
    [DigiviceFormKoloso] = mon_koloso,
    [DigiviceFormAureola] = mon_aureola,
    [DigiviceFormDrakstat] = mon_drakstat,
    [DigiviceFormOrakulo] = mon_orakulo,
    [DigiviceFormNularca] = mon_nularca,
    [DigiviceFormPrismar] = mon_prismar,
    [DigiviceFormBaliza] = mon_baliza,
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
