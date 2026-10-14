#include "digivice_app_i.h"

// Frame marks of the care animations, at 10 fps.
#define CLEAN_SWEEP_END  12
#define LIGHT_YAWN_END   6
#define LIGHT_DIM_END    14
#define CURE_SLIDE_END   8
#define CURE_PUSH_END    16
#define CURE_SPARKLE_END 22

void digivice_care_start(DigiviceApp* app, DigiviceCare care) {
    app->care = care;
    app->care_frame = 0;
    app->notice_frames = 0;
}

void digivice_care_tick(DigiviceApp* app) {
    app->care_frame++;
    if(app->care_frame >= DIGIVICE_CARE_FRAMES) {
        app->care = DigiviceCareNone;
        digivice_app_refresh(app);
    }
}

static void digivice_care_hearts(Canvas* canvas, int32_t x, uint32_t t) {
    for(int32_t i = 0; i < 2; i++) {
        int32_t q = (int32_t)((t + (uint32_t)i * 5) % 10);
        digivice_draw_heart(canvas, x + 10 + i * 8, 24 - q);
    }
}

static void digivice_care_clean(Canvas* canvas, DigiviceApp* app, int32_t pet_x) {
    uint32_t f = app->care_frame;
    if(f < CLEAN_SWEEP_END) {
        int32_t x = 128 - (int32_t)f * 140 / CLEAN_SWEEP_END;
        digivice_draw_sweep(canvas, x, 13, 52, app->frame);
    } else {
        digivice_care_hearts(canvas, pet_x, f - CLEAN_SWEEP_END);
        digivice_draw_label(canvas, 14, digivice_app_str(app, DigiviceStrCleanDone));
    }
}

static uint8_t digivice_care_dark_level(const DigiviceApp* app) {
    uint32_t f = app->care_frame;
    const uint32_t dim = LIGHT_DIM_END - LIGHT_YAWN_END;
    // Lights off: a yawn, then the band dims to 12/16. Lights on: it brightens straight away.
    if(app->care == DigiviceCareLightOff) {
        if(f < LIGHT_YAWN_END) return 0;
        if(f >= LIGHT_DIM_END) return 12;
        return (uint8_t)((f - LIGHT_YAWN_END) * 12 / dim);
    }
    return f < dim ? (uint8_t)(12 - f * 12 / dim) : 0;
}

static void digivice_care_light(Canvas* canvas, DigiviceApp* app, int32_t pet_x) {
    uint32_t f = app->care_frame;
    bool off = app->care == DigiviceCareLightOff;
    uint8_t level = digivice_care_dark_level(app);
    if(level) digivice_draw_dark(canvas, 12, 52, level, app->frame);

    canvas_set_color(canvas, level >= 12 ? ColorWhite : ColorBlack);
    digivice_draw_zzz(canvas, pet_x + 12, 26, app->frame);
    canvas_set_color(canvas, ColorBlack);

    if(off && f < LIGHT_YAWN_END) {
        digivice_draw_label(canvas, 14, digivice_app_str(app, DigiviceStrLightYawn));
    } else if(off && f >= LIGHT_DIM_END) {
        digivice_draw_label(canvas, 14, digivice_app_str(app, DigiviceStrLightNight));
    } else if(!off && level == 0) {
        digivice_draw_label(canvas, 14, digivice_app_str(app, DigiviceStrLightOn));
    }
}

static void digivice_care_cure(Canvas* canvas, DigiviceApp* app, int32_t pet_x) {
    uint32_t f = app->care_frame;
    bool cured = !app->state.sick;
    if(f < CURE_PUSH_END) {
        int32_t slide = f < CURE_SLIDE_END ? (int32_t)f : CURE_SLIDE_END;
        int32_t x = 120 - slide * 52 / CURE_SLIDE_END;
        int32_t push = f < CURE_SLIDE_END ? 0 : (int32_t)(f - CURE_SLIDE_END);
        digivice_draw_syringe(canvas, x - 6, 37, push);
    }
    // The skull keeps showing until the sparkles, then only if a dose is still missing.
    if(cured && f < CURE_PUSH_END) digivice_draw_skull(canvas, pet_x - 3, 18);
    if(f >= CURE_SLIDE_END + 6 && f < CURE_SPARKLE_END) {
        digivice_draw_sparkle_ring(canvas, pet_x, 34, 16, 10, f);
    }
    if(f >= CURE_SPARKLE_END - 4) {
        if(cured) digivice_care_hearts(canvas, pet_x, f);
        digivice_draw_label(
            canvas,
            14,
            digivice_app_str(app, cured ? DigiviceStrCureDone : DigiviceStrCureMoreDose));
    }
}

void digivice_care_draw(Canvas* canvas, DigiviceApp* app, int32_t pet_x) {
    switch(app->care) {
    case DigiviceCareClean:
        digivice_care_clean(canvas, app, pet_x);
        break;
    case DigiviceCareLightOff:
    case DigiviceCareLightOn:
        digivice_care_light(canvas, app, pet_x);
        break;
    case DigiviceCareCure:
        digivice_care_cure(canvas, app, pet_x);
        break;
    default:
        break;
    }
}
