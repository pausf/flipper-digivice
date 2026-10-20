#include "digivice_draw.h"
#include "digivice_city.h"
#include "digivice_config.h"

#define CITY_GROUND 50
#define CITY_LOOP   DIGIVICE_SCENE_W

typedef enum {
    LmNone,
    LmSagrada,
    LmGlories,
    LmAlcala,
    LmCuatro,
    LmDome,
    LmHelmet,
    LmGiralda,
    LmTower,
    LmGuggenheim,
    LmArch,
} DvLandmark;

typedef struct {
    uint8_t kind;
    uint8_t x; /**< left edge on the 192 px loop */
} DvPlace;

// Two landmarks per city, 96 px apart so one is always on screen. Row 0 is city 1.
static const DvPlace panos[DIGIVICE_CITY_COUNT][2] = {
    {{LmSagrada, 20}, {LmGlories, 124}},
    {{LmAlcala, 24}, {LmCuatro, 120}},
    {{LmDome, 20}, {LmHelmet, 116}},
    {{LmGiralda, 30}, {LmTower, 126}},
    {{LmGuggenheim, 16}, {LmArch, 112}},
};

static void lm_sagrada(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    static const uint8_t h[4] = {22, 28, 30, 24};
    (void)frame;
    for(int32_t i = 0; i < 4; i++) {
        int32_t tx = x + 2 + i * 8;
        canvas_set_color(c, ink);
        digivice_draw_box(c, tx, g - h[i], 6, h[i]);
        digivice_draw_fill_triangle(c, tx, g - h[i], tx + 5, g - h[i], tx + 2, g - h[i] - 7);
        canvas_set_color(c, paper);
        digivice_draw_line(c, tx + 2, g - h[i] + 4, tx + 2, g - h[i] + 8);
        digivice_draw_line(c, tx + 2, g - h[i] + 12, tx + 2, g - h[i] + 15);
    }
    canvas_set_color(c, ink);
    digivice_draw_box(c, x, g - 12, 36, 12);
    canvas_set_color(c, paper);
    for(int32_t i = 0; i < 3; i++) digivice_draw_box(c, x + 4 + i * 11, g - 8, 4, 8);
}

static void lm_glories(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    canvas_set_color(c, ink);
    digivice_draw_fill_ellipse(c, x + 6, g - 20, 6, 18);
    digivice_draw_box(c, x, g - 4, 12, 4);
    digivice_draw_line(c, x + 6, g - 38, x + 6, g - 36);
    canvas_set_color(c, paper);
    bool night = ink == ColorWhite;
    int32_t shift = (int32_t)((frame / 8) % 2);
    int32_t row = 0;
    for(int32_t y = g - 34; y <= g - 6; y += 3, row++) {
        int32_t col = 0;
        for(int32_t dx = 2; dx <= 10; dx += 3, col++) {
            if(night && (row + col) % 2) continue; // lit windows
            int32_t px = dx + (row % 2 ? shift : 0);
            if(px > 10) px = 10;
            digivice_draw_dot(c, x + px, y);
        }
    }
}

static void lm_alcala(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    (void)frame;
    canvas_set_color(c, ink);
    digivice_draw_box(c, x, g - 18, 30, 18);
    digivice_draw_box(c, x + 4, g - 22, 22, 4);
    digivice_draw_box(c, x + 12, g - 26, 6, 4);
    canvas_set_color(c, paper);
    digivice_draw_box(c, x + 3, g - 10, 5, 10);
    digivice_draw_fill_ellipse(c, x + 5, g - 10, 2, 2);
    digivice_draw_box(c, x + 12, g - 12, 6, 12);
    digivice_draw_fill_ellipse(c, x + 15, g - 12, 3, 3);
    digivice_draw_box(c, x + 22, g - 10, 5, 10);
    digivice_draw_fill_ellipse(c, x + 24, g - 10, 2, 2);
}

static void lm_cuatro(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    static const uint8_t h[4] = {38, 34, 30, 36};
    (void)frame;
    for(int32_t i = 0; i < 4; i++) {
        int32_t tx = x + i * 9;
        int32_t hh = h[i] > g - 12 ? g - 12 : h[i];
        canvas_set_color(c, ink);
        digivice_draw_box(c, tx, g - hh, 6, hh);
        digivice_draw_line(c, tx, g - hh - 1, tx + 5, g - hh - 1);
        canvas_set_color(c, paper);
        for(int32_t y = g - hh + 3; y < g - 2; y += 4) digivice_draw_line(c, tx + 1, y, tx + 4, y);
    }
}

static void lm_dome(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    (void)frame;
    canvas_set_color(c, ink);
    digivice_draw_fill_ellipse(c, x + 14, g - 10, 14, 10);
    canvas_set_color(c, paper);
    digivice_draw_fill_ellipse(c, x + 14, g - 10, 6, 5);
    canvas_set_color(c, ink);
    digivice_draw_dot(c, x + 14, g - 10);
    for(int32_t k = 0; k <= 26; k += 4) digivice_draw_line(c, x + k, g - 1, x + k + 1, g - 1);
}

static void lm_helmet(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    (void)frame;
    canvas_set_color(c, ink);
    digivice_draw_box(c, x + 4, g - 8, 24, 8);
    digivice_draw_fill_ellipse(c, x + 16, g - 14, 14, 9);
    digivice_draw_fill_triangle(c, x + 2, g - 20, x + 16, g - 34, x + 30, g - 18);
    canvas_set_color(c, paper);
    digivice_draw_line(c, x + 10, g - 20, x + 22, g - 26);
}

static void lm_giralda(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    canvas_set_color(c, ink);
    digivice_draw_box(c, x + 6, g - 24, 8, 24); // shaft
    digivice_draw_box(c, x + 4, g - 31, 12, 7); // belfry
    digivice_draw_box(c, x + 7, g - 36, 6, 5); // lantern
    digivice_draw_line(c, x + 10, g - 36, x + 10, g - 41); // spire
    int32_t vane = (int32_t)((frame / 4) % 2) * 2 - 1; // the weathervane turns every 400 ms
    digivice_draw_line(c, x + 10, g - 40, x + 10 + 3 * vane, g - 42);
    canvas_set_color(c, paper);
    digivice_draw_line(c, x + 10, g - 20, x + 10, g - 16); // slits
    digivice_draw_line(c, x + 10, g - 12, x + 10, g - 8);
    digivice_draw_box(c, x + 6, g - 29, 2, 3); // belfry arches
    digivice_draw_box(c, x + 12, g - 29, 2, 3);
}

static void lm_tower(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    (void)frame;
    canvas_set_color(c, ink);
    digivice_draw_box(c, x + 4, g - 18, 14, 18);
    digivice_draw_box(c, x + 6, g - 26, 10, 8);
    digivice_draw_box(c, x + 8, g - 31, 6, 5);
    for(int32_t k = 0; k < 4; k++) digivice_draw_box(c, x + 4 + k * 4, g - 20, 2, 2);
    canvas_set_color(c, paper);
    digivice_draw_box(c, x + 9, g - 14, 2, 3);
    digivice_draw_box(c, x + 9, g - 24, 2, 3);
}

static void lm_guggenheim(
    Canvas* c,
    int32_t x,
    int32_t g,
    Color ink,
    Color paper,
    uint32_t frame) {
    static const int8_t petals[3][4] = {{8, 8, 9, 7}, {20, 10, 10, 9}, {31, 7, 8, 6}};
    (void)frame;
    canvas_set_color(c, ink);
    for(int32_t i = 0; i < 3; i++) {
        digivice_draw_fill_ellipse(
            c, x + petals[i][0], g - petals[i][1], petals[i][2], petals[i][3]);
    }
    digivice_draw_box(c, x, g - 3, 40, 3);
    canvas_set_color(c, paper);
    for(int32_t i = 0; i < 3; i++) {
        int32_t cx = x + petals[i][0];
        int32_t cy = g - petals[i][1];
        for(int32_t k = 3 - petals[i][2]; k < petals[i][2] - 2; k += 3) {
            digivice_draw_line(c, cx + k, cy - 1, cx + k + 1, cy + 1);
        }
    }
}

static void lm_arch(Canvas* c, int32_t x, int32_t g, Color ink, Color paper, uint32_t frame) {
    (void)frame;
    canvas_set_color(c, ink);
    digivice_draw_fill_ellipse(c, x + 20, g, 20, 22);
    canvas_set_color(c, paper);
    digivice_draw_fill_ellipse(c, x + 20, g, 17, 19);
    digivice_draw_box(c, x - 1, g + 1, 42, 24); // the ellipses' lower halves belong to the street
    canvas_set_color(c, ink);
    digivice_draw_box(c, x, g - 3, 40, 3);
    for(int32_t k = 0; k < 5; k++) digivice_draw_line(c, x + 8 + k * 6, g - 3, x + 8 + k * 6, g - 14);
}

static void draw_landmark(Canvas* c, uint8_t kind, int32_t x, Color ink, Color paper, uint32_t frame) {
    switch(kind) {
    case LmSagrada: lm_sagrada(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmGlories: lm_glories(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmAlcala: lm_alcala(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmCuatro: lm_cuatro(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmDome: lm_dome(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmHelmet: lm_helmet(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmGiralda: lm_giralda(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmTower: lm_tower(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmGuggenheim: lm_guggenheim(c, x, CITY_GROUND, ink, paper, frame); break;
    case LmArch: lm_arch(c, x, CITY_GROUND, ink, paper, frame); break;
    default:
        break;
    }
}

void digivice_draw_city(
    Canvas* canvas,
    const DigiviceScene* scene,
    uint32_t scroll,
    uint32_t frame,
    bool dark) {
    Color ink = dark ? ColorWhite : ColorBlack;
    Color paper = dark ? ColorBlack : ColorWhite;
    uint32_t far_scroll = scroll / 4;
    int32_t sub = (int32_t)(far_scroll % 6);

    // Far layer: 5 px blocks on a 6 px pitch, outlined, with windows (lit at night).
    for(int32_t k = 0; k < 23; k++) {
        int32_t x = k * 6 - sub;
        uint32_t col = ((far_scroll / 6 + (uint32_t)k) * 6) % CITY_LOOP;
        int32_t top = scene->far[col];
        canvas_set_color(canvas, paper);
        digivice_draw_box(canvas, x, top, 5, CITY_GROUND - top);
        canvas_set_color(canvas, ink);
        digivice_draw_line(canvas, x, top, x + 4, top);
        digivice_draw_line(canvas, x, top, x, CITY_GROUND - 1);
        digivice_draw_line(canvas, x + 4, top, x + 4, CITY_GROUND - 1);
        for(int32_t y = top + 3; y < CITY_GROUND - 2; y += 4) {
            if(dark && ((col + (uint32_t)y) % 3) == 0) continue;
            digivice_draw_dot(canvas, x + 2, y);
        }
    }

    // Mid layer: the two landmarks of the city at half speed on the 192 px loop.
    uint32_t mid = scroll / 2;
    for(int32_t i = 0; i < 2; i++) {
        const DvPlace* place = &panos[scene->city - 1][i];
        int32_t x = (int32_t)((place->x + CITY_LOOP - mid % CITY_LOOP) % CITY_LOOP);
        if(x > 140) x -= CITY_LOOP;
        if(x < -44) continue;
        draw_landmark(canvas, place->kind, x, ink, paper, frame);
    }

    // Near layer: ground line, street dots and a lamp post every 32 px at full speed.
    canvas_set_color(canvas, ink);
    digivice_draw_line(canvas, 0, CITY_GROUND, 127, CITY_GROUND);
    for(int32_t x = 5 - (int32_t)(scroll % 6); x < 128; x += 6) {
        digivice_draw_dot(canvas, x, CITY_GROUND + 3 + (x / 6) % 3);
    }
    for(int32_t x = 12 - (int32_t)(scroll % 32); x < 140; x += 32) {
        digivice_draw_line(canvas, x, CITY_GROUND - 9, x, CITY_GROUND);
        digivice_draw_box(canvas, x - 1, CITY_GROUND - 10, 3, 2);
    }
    canvas_set_color(canvas, ColorBlack);
}

// Interim: Task 10 replaces this attribute mapping with the real sprites.
bool digivice_draw_city_monster(
    Canvas* canvas,
    uint8_t idx,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    DigivicePose pose,
    bool flip,
    bool silhouette) {
    if(idx >= DIGIVICE_CITY_COUNT) return false;
    static const uint8_t by_attr[DigiviceAttrCount] = {
        DigiviceFoeKorno, DigiviceFoeFuzzcan, DigiviceFoeChipko, DigiviceFoeMokobit};
    digivice_draw_foe(
        canvas,
        (DigiviceFoe)by_attr[digivice_city_monster_attr(idx)],
        cx,
        foot_y,
        frame,
        pose,
        flip,
        silhouette);
    return true;
}
