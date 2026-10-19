#include "digivice_draw.h"
#include "digivice_config.h"

#define SKY_TOP 9

typedef enum {
    DecoGrass,
    DecoTree,
    DecoMast,
    DecoIce,
    DecoMagnet,
    DecoTower,
    DecoPacket,
    DecoShard,
} DvDeco;

typedef enum {
    NearOutline,
    NearSolid,
    NearStripes,
} DvNear;

typedef struct {
    uint8_t far_base;
    uint8_t far_amp;
    uint8_t near_base;
    uint8_t near_amp;
    uint8_t far_step; /**< far layer: a fill line every far_step columns, dots between */
    uint8_t near; /**< DvNear */
    uint8_t ground; /**< ground line row */
    uint8_t deco; /**< DvDeco */
} DvLook;

// Ported from the mockup's bgMeadow ... bgNull, reduced to two cached profiles per area.
static const DvLook looks[DIGIVICE_AREA_COUNT] = {
    {30, 4, 40, 3, 6, NearStripes, 47, DecoGrass},
    {26, 3, 41, 2, 6, NearOutline, 48, DecoTree},
    {28, 6, 43, 4, 3, NearSolid, 54, DecoMast},
    {30, 5, 40, 3, 6, NearOutline, 48, DecoIce},
    {30, 3, 38, 4, 6, NearStripes, 48, DecoMagnet},
    {40, 2, 46, 3, 6, NearSolid, 54, DecoTower},
    {28, 3, 52, 2, 3, NearStripes, 56, DecoPacket},
    {34, 4, 46, 3, 3, NearOutline, 50, DecoShard},
};

static uint32_t scene_rng(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void profile(uint8_t* out, uint8_t base, uint8_t amp, uint32_t* rng) {
    // Value noise: a control point every 16 columns, linear in between, wrapping at 192.
    uint8_t points[DIGIVICE_SCENE_W / 16];
    for(uint32_t i = 0; i < DIGIVICE_SCENE_W / 16; i++) {
        points[i] = (uint8_t)(base - amp + scene_rng(rng) % (2U * amp + 1U));
    }
    for(uint32_t x = 0; x < DIGIVICE_SCENE_W; x++) {
        int32_t a = points[x / 16];
        int32_t b = points[(x / 16 + 1) % (DIGIVICE_SCENE_W / 16)];
        out[x] = (uint8_t)(a + (b - a) * (int32_t)(x % 16) / 16);
    }
}

void digivice_scene_init(DigiviceScene* scene, uint8_t area) {
    if(area >= DIGIVICE_AREA_COUNT) area = DIGIVICE_AREA_COUNT - 1;
    const DvLook* look = &looks[area];
    scene->area = area;
    scene->seed = 0xD1610UL ^ ((uint32_t)area * 0x9E37UL);
    uint32_t rng = scene->seed;
    profile(scene->far, look->far_base, look->far_amp, &rng);
    profile(scene->near, look->near_base, look->near_amp, &rng);
    for(uint32_t i = 0; i < DIGIVICE_SCENE_DECOS; i++) {
        scene->deco_x[i] = (uint8_t)(i * 16 + scene_rng(&rng) % 12);
        scene->deco_v[i] = (uint8_t)(scene_rng(&rng) % 4);
    }
}

static void sky_day(Canvas* canvas, uint32_t scroll) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_ring(canvas, 110, 16, 4, 4);
    for(int32_t i = 0; i < 3; i++) {
        int32_t x = (int32_t)(((uint32_t)i * 53 + 170 - scroll / 8 % 170) % 170) - 20;
        digivice_draw_ring(canvas, x, 13 + i * 4, 9, 3);
    }
}

static void sky_dusk(Canvas* canvas, uint8_t sun_row) {
    canvas_set_color(canvas, ColorBlack);
    for(int32_t y = SKY_TOP; y < SKY_TOP + 8; y += 2) {
        for(int32_t x = (y / 2) % 4; x < 128; x += 4)
            canvas_draw_dot(canvas, x, y);
    }
    digivice_draw_fill_ellipse(canvas, 100, sun_row, 5, 5);
}

static void sky_night(Canvas* canvas, const DigiviceScene* scene, uint32_t frame, uint8_t low) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_box(canvas, 0, SKY_TOP, 128, 64 - SKY_TOP);
    canvas_set_color(canvas, ColorWhite);
    // 26 stars from the cached seed, a few twinkling off each frame.
    uint32_t rng = scene->seed | 1U;
    for(uint32_t i = 0; i < 26; i++) {
        uint32_t r = scene_rng(&rng);
        if((r >> 24) % 7 == frame % 7) continue;
        digivice_draw_dot(canvas, (int32_t)(r % 128), SKY_TOP + 1 + (int32_t)((r >> 8) % low));
    }
    digivice_draw_fill_ellipse(canvas, 100, 15, 4, 4);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_fill_ellipse(canvas, 102, 14, 3, 3);
}

// One decoration standing on `base`, in `ink` over `paper`.
static void deco(
    Canvas* canvas,
    const DvLook* look,
    uint8_t v,
    int32_t x,
    int32_t base,
    uint32_t frame,
    bool dark) {
    Color ink = dark ? ColorWhite : ColorBlack;
    Color paper = dark ? ColorBlack : ColorWhite;
    canvas_set_color(canvas, ink);
    switch(look->deco) {
    case DecoGrass:
        digivice_draw_line(canvas, x, base, x - 1, base - 4);
        digivice_draw_line(canvas, x + 2, base, x + 3, base - 5);
        digivice_draw_line(canvas, x + 4, base, x + 4, base - 3);
        break;
    case DecoTree:
        // Wire tree with solder points.
        digivice_draw_box(canvas, x - 1, base - 18, 2, 18);
        digivice_draw_line(canvas, x, base - 12, x - 6, base - 19);
        digivice_draw_line(canvas, x, base - 15, x + 6, base - 22);
        digivice_draw_line(canvas, x, base - 18, x - 3, base - 25);
        digivice_draw_box(canvas, x - 7, base - 20, 2, 2);
        digivice_draw_box(canvas, x + 5, base - 23, 2, 2);
        digivice_draw_box(canvas, x - 4, base - 26, 2, 2);
        break;
    case DecoMast:
        digivice_draw_line(canvas, x, base - 20, x - 4, base);
        digivice_draw_line(canvas, x, base - 20, x + 4, base);
        digivice_draw_line(canvas, x - 3, base - 6, x + 3, base - 12);
        digivice_draw_line(canvas, x + 3, base - 6, x - 3, base - 12);
        digivice_draw_line(canvas, x, base - 24, x, base - 20);
        // Single-frame pulse on the 100 ms clock.
        if((frame + v) % 5 == 0) digivice_draw_burst(canvas, x, base - 25, 5);
        break;
    case DecoIce: {
        int32_t w = 7 + v * 2;
        canvas_set_color(canvas, paper);
        digivice_draw_box(canvas, x, base - w, w, w);
        canvas_set_color(canvas, ink);
        // Frames as clipped lines: x goes negative while a decoration scrolls off the left edge.
        digivice_draw_line(canvas, x, base - w, x + w - 1, base - w);
        digivice_draw_line(canvas, x, base - 1, x + w - 1, base - 1);
        digivice_draw_line(canvas, x, base - w, x, base - 1);
        digivice_draw_line(canvas, x + w - 1, base - w, x + w - 1, base - 1);
        digivice_draw_line(canvas, x + 2, base - 3, x + w - 3, base - w + 2);
        break;
    }
    case DecoMagnet:
        digivice_draw_box(canvas, x - 5, base - 9, 3, 9);
        digivice_draw_box(canvas, x + 2, base - 9, 3, 9);
        digivice_draw_box(canvas, x - 5, base - 11, 10, 3);
        digivice_draw_dot(canvas, x - 8, base - 2 - (int32_t)(frame % 3));
        digivice_draw_dot(canvas, x + 8, base - 3 + (int32_t)(frame % 3));
        break;
    case DecoTower:
        if(v != 0) {
            digivice_draw_line(canvas, x, base - 10, x, base);
            digivice_draw_dot(canvas, x, base - 11);
            break;
        }
        digivice_draw_line(canvas, x - 2, 14, x - 10, base);
        digivice_draw_line(canvas, x + 2, 14, x + 10, base);
        for(int32_t y = 20; y < base; y += 8) {
            int32_t w = 2 + (y - 14) * 8 / (base - 14);
            digivice_draw_line(canvas, x - w, y, x + w, y + 8 < base ? y + 8 : base);
        }
        digivice_draw_line(canvas, x, 14, x, 8);
        if(frame % 6 < 3) {
            // Strobe: a single-frame toggle on the 100 ms clock.
            digivice_draw_fill_ellipse(canvas, x, 8, 2, 2);
            digivice_draw_burst(canvas, x, 8, 7);
        } else {
            digivice_draw_ring(canvas, x, 8, 2, 2);
        }
        break;
    case DecoPacket:
        canvas_set_color(canvas, paper);
        digivice_draw_box(canvas, x - 6, base - 12, 13, 9);
        canvas_set_color(canvas, ink);
        digivice_draw_line(canvas, x - 6, base - 12, x + 6, base - 12);
        digivice_draw_line(canvas, x - 6, base - 4, x + 6, base - 4);
        digivice_draw_line(canvas, x - 6, base - 12, x - 6, base - 4);
        digivice_draw_line(canvas, x + 6, base - 12, x + 6, base - 4);
        digivice_draw_text(canvas, x - 3, base - 11, v % 2 ? "01" : "10");
        break;
    default: {
        // Floating shards of the null core, bobbing.
        int32_t y = 14 + v * 6 + (int32_t)((frame / 4 + v) % 3);
        int32_t s = 1 + v;
        digivice_draw_line(canvas, x, y, x + s, y);
        digivice_draw_line(canvas, x, y + s, x + s, y + s);
        digivice_draw_line(canvas, x, y, x, y + s);
        digivice_draw_line(canvas, x + s, y, x + s, y + s);
        break;
    }
    }
}

void digivice_draw_scene(
    Canvas* canvas,
    const DigiviceScene* scene,
    uint32_t scroll,
    uint32_t frame,
    uint16_t day_minute) {
    const DvLook* look = &looks[scene->area < DIGIVICE_AREA_COUNT ? scene->area : 0];
    bool night = day_minute >= 20 * 60 || day_minute < 6 * 60;
    bool dusk = !night && (day_minute >= 18 * 60 || day_minute < 7 * 60 + 30);
    // The null core is a negative world: always drawn with the night palette.
    bool dark = night || scene->area == DIGIVICE_AREA_COUNT - 1;

    if(dark) {
        sky_night(canvas, scene, frame, (uint8_t)(look->far_base - SKY_TOP - 2));
    } else if(dusk) {
        sky_dusk(canvas, (uint8_t)(look->far_base - 1));
    } else {
        sky_day(canvas, scroll);
    }

    Color ink = dark ? ColorWhite : ColorBlack;
    uint32_t far_off = scroll / 3;
    for(int32_t x = 0; x < 128; x++) {
        int32_t fh = scene->far[(far_off + (uint32_t)x) % DIGIVICE_SCENE_W];
        int32_t nh = scene->near[(scroll + (uint32_t)x) % DIGIVICE_SCENE_W];
        canvas_set_color(canvas, ink);
        // The far layer only shows above the near one: no overdraw, one call per layer.
        if(fh < nh) {
            if(x % look->far_step == 0) {
                digivice_draw_line(canvas, x, fh, x, nh - 1);
            } else {
                digivice_draw_dot(canvas, x, fh);
            }
        }
        if(look->near == NearSolid || (look->near == NearStripes && x % 2 == 0)) {
            // At night the near ground stays dark and only its outline is light.
            canvas_set_color(canvas, ColorBlack);
            digivice_draw_line(canvas, x, nh, x, look->ground);
            if(dark) {
                canvas_set_color(canvas, ColorWhite);
                digivice_draw_dot(canvas, x, nh);
            }
        } else {
            digivice_draw_dot(canvas, x, nh);
        }
    }

    canvas_set_color(canvas, ink);
    digivice_draw_line(canvas, 0, look->ground, 127, look->ground);
    for(int32_t x = 5 - (int32_t)(scroll % 6); x < 128; x += 6)
        digivice_draw_dot(canvas, x, look->ground + 3 + (x / 6) % 3);
    if(look->deco == DecoGrass) {
        // TV-snow grass: the noise boils with the frame, not with a per-pixel hash.
        uint32_t rng = scene->seed ^ (frame * 0x9E3779B9UL);
        for(uint32_t i = 0; i < 24; i++) {
            uint32_t r = scene_rng(&rng);
            int32_t y = look->ground + 1 + (int32_t)((r >> 8) % (63U - look->ground));
            digivice_draw_dot(canvas, (int32_t)(r % 128), y);
        }
    }

    for(uint32_t i = 0; i < DIGIVICE_SCENE_DECOS; i++) {
        int32_t x = (int32_t)((scene->deco_x[i] + DIGIVICE_SCENE_W - scroll % DIGIVICE_SCENE_W) %
                              DIGIVICE_SCENE_W);
        if(x > 140) x -= DIGIVICE_SCENE_W;
        if(x < -12 || x > 140) continue;
        int32_t base = x >= 0 && x < 128 ? scene->near[(scroll + (uint32_t)x) % DIGIVICE_SCENE_W] :
                                           look->near_base;
        deco(canvas, look, scene->deco_v[i], x, base, frame, dark);
    }
    canvas_set_color(canvas, ColorBlack);
}
