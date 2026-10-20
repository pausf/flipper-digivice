#include "digivice_app_i.h"

#include <furi_hal.h>
#include <stdio.h>

#define BAR_SEGMENTS 12

// Bars grow in one after another over the first frames, as in the mockup.
static uint8_t digivice_profile_grow(uint32_t frame, uint8_t row, uint8_t segments) {
    uint32_t start = row * 2U;
    if(frame <= start) return 0;
    uint32_t shown = (frame - start) * BAR_SEGMENTS / 8;
    return shown < segments ? (uint8_t)shown : segments;
}

static void digivice_profile_bar(
    Canvas* canvas,
    DigiviceApp* app,
    uint8_t row,
    DigiviceStr label,
    uint32_t value,
    uint32_t max) {
    int32_t y = 13 + row * 9;
    uint8_t full = (uint8_t)((value > max ? max : value) * BAR_SEGMENTS / max);
    uint8_t shown = digivice_profile_grow(app->screen_frame, row, full);
    digivice_draw_text(canvas, 48, y, digivice_app_str(app, label));
    for(uint8_t j = 0; j < BAR_SEGMENTS; j++) {
        int32_t sx = 74 + j * 4;
        if(j < shown) {
            canvas_draw_box(canvas, sx, y, 3, 5);
        } else {
            canvas_draw_box(canvas, sx, y + 4, 3, 1);
        }
    }
}

/** Time in the current form toward its evolution, as 0..62 px of the bar. */
static uint8_t digivice_profile_exp_px(const DigiviceState* s) {
    const DigiviceRosterEntry* e = digivice_roster(s->form);
    if(!e || e->stay_s == 0) return 0;
    uint32_t now = furi_hal_rtc_get_timestamp();
    uint32_t elapsed = now > s->stage_since ? now - s->stage_since : 0;
    return elapsed >= e->stay_s ? 62 : (uint8_t)(elapsed * 62UL / e->stay_s);
}

void digivice_profile_draw(Canvas* canvas, DigiviceApp* app) {
    const DigiviceState* s = &app->state;
    uint32_t f = app->screen_frame;
    canvas_set_color(canvas, ColorBlack);

    digivice_app_draw_title(canvas, digivice_form_name((DigiviceForm)s->form));
    const char* mode = digivice_app_str(
        app, s->mode == DigiviceModeHacker ? DigiviceStrModeHacker : DigiviceStrModeNormal);
    digivice_draw_text(canvas, 125 - digivice_draw_text_width(canvas, mode), 2, mode);

    digivice_draw_brackets(canvas, 2, 11, 42, 46, 4);
    digivice_draw_dash_ring(canvas, 22, 29, 16, app->frame / 2);
    digivice_draw_form(
        canvas, (DigiviceForm)s->form, 22, 32, app->frame, DigivicePoseIdle, false);
    digivice_draw_text(canvas, 4, 49, digivice_type_name(digivice_rules_player_type(s)));

    // Label (6 glyphs, 12 bytes at most) + space + a uint8_t + NUL.
    char line[20];
    snprintf(
        line,
        sizeof(line),
        "%s %u",
        digivice_app_str(app, DigiviceStrProfileMistakes),
        (unsigned)s->care_mistakes);
    digivice_draw_text(canvas, 4, 57, line);

    digivice_profile_bar(canvas, app, 0, DigiviceStrProfileHunger, s->hunger, DIGIVICE_HUNGER_MAX);
    digivice_profile_bar(canvas, app, 1, DigiviceStrProfileMood, s->mood, DIGIVICE_MOOD_MAX);
    digivice_profile_bar(
        canvas, app, 2, DigiviceStrProfileCorruption, s->corruption, DIGIVICE_CORRUPTION_MAX);
    digivice_profile_bar(
        canvas, app, 3, DigiviceStrProfileStrength, s->strength, DIGIVICE_STRENGTH_BAR_MAX);

    digivice_draw_text(canvas, 48, 49, digivice_app_str(app, DigiviceStrExp));
    canvas_draw_frame(canvas, 62, 49, 64, 5);
    uint8_t exp_px = digivice_profile_exp_px(s);
    uint32_t grow = f < 10 ? f * exp_px / 10 : exp_px;
    if(grow) canvas_draw_box(canvas, 63, 50, grow, 3);

    // Data per type only matters in Hacker mode, where deeds feed it.
    if(s->mode == DigiviceModeHacker) {
        uint16_t top = 1;
        for(uint8_t t = 0; t < DigiviceTypeCount; t++) {
            if(s->data[t] > top) top = s->data[t];
        }
        for(uint8_t t = 0; t < DigiviceTypeCount; t++) {
            int32_t x = 48 + t * 20;
            char initial[2] = {digivice_type_name((DigiviceType)t)[0], '\0'};
            digivice_draw_text(canvas, x, 57, initial);
            int32_t h = (int32_t)((uint32_t)s->data[t] * 5 / top);
            if(f < 20) h = h * (int32_t)f / 20;
            if(h > 0) canvas_draw_box(canvas, x + 5, 63 - h, 8, h);
        }
    }

    if(f % 30 == 0) digivice_draw_glitch(canvas, app->frame, 2);
}
