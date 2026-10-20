#include "digivice_app_i.h"

#include <digivice/digivice_config.h>
#include <furi_hal.h>

#if DIGIVICE_EASTER_EGGS

#define NIDO_FLOOR 47

static void slot(uint8_t id, int32_t* x, int32_t* foot) {
    *x = 14 + (id % 5) * 25;
    *foot = id < 5 ? 31 : NIDO_FLOOR;
}

// The next caught partner after `from` in direction `dir`; `from` itself when it is the only one.
static uint8_t nido_step(uint16_t caught, uint8_t from, int32_t dir) {
    for(int32_t i = 1; i <= DIGIVICE_HOMAGE_COUNT; i++) {
        int32_t next = (from + dir * i + 2 * DIGIVICE_HOMAGE_COUNT) % DIGIVICE_HOMAGE_COUNT;
        uint8_t id = (uint8_t)next;
        if(caught & (1U << id)) return id;
    }
    return from;
}

void digivice_nido_enter(DigiviceApp* app) {
    uint16_t caught = app->state.caught;
    uint8_t first = app->state.companion < DIGIVICE_HOMAGE_COUNT ? app->state.companion : 0;
    app->nido_sel = caught & (1U << first) ? first : nido_step(caught, first, 1);
    digivice_app_set_screen(app, DigiviceScreenNido);
}

void digivice_nido_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return;
    uint16_t caught = app->state.caught;
    switch(event->key) {
    case InputKeyRight:
        app->nido_sel = nido_step(caught, app->nido_sel, 1);
        break;
    case InputKeyLeft:
        app->nido_sel = nido_step(caught, app->nido_sel, -1);
        break;
    case InputKeyOk:
        if(event->type != InputTypeShort || !(caught & (1U << app->nido_sel))) break;
        digivice_set_companion(
            app->digivice,
            app->state.companion == app->nido_sel ? DIGIVICE_COMPANION_NONE : app->nido_sel);
        digivice_get_state(app->digivice, &app->state);
        break;
    case InputKeyBack:
        digivice_app_set_screen(app, DigiviceScreenProfile);
        break;
    default:
        break;
    }
}

void digivice_nido_draw(Canvas* canvas, DigiviceApp* app) {
    const DigiviceState* s = &app->state;
    bool asleep = digivice_rules_is_asleep(furi_hal_rtc_get_timestamp());
    canvas_set_color(canvas, ColorBlack);
    digivice_app_draw_title(canvas, digivice_app_str(app, DigiviceStrNidoTitle));

    // The room: floor, window, rug and lamp.
    for(int32_t x = 0; x < 128; x += 2)
        canvas_draw_dot(canvas, x, NIDO_FLOOR + 1);
    canvas_draw_frame(canvas, 98, 12, 24, 14);
    canvas_draw_line(canvas, 110, 12, 110, 25);
    digivice_draw_dither(canvas, 20, NIDO_FLOOR + 3, 70, 2, 8);
    canvas_draw_line(canvas, 8, 40, 8, NIDO_FLOOR);
    canvas_draw_box(canvas, 5, 38, 7, 3);

    uint16_t caught = s->caught;
    if(!caught) {
        const char* hint = digivice_app_str(app, DigiviceStrNidoEmpty);
        digivice_draw_text(canvas, 64 - digivice_draw_text_width(canvas, hint) / 2, 55, hint);
        return;
    }
    for(uint8_t id = 0; id < DIGIVICE_HOMAGE_COUNT; id++) {
        if(!(caught & (1U << id))) continue;
        int32_t x;
        int32_t foot;
        slot(id, &x, &foot);
        // Each one drifts a little, out of phase with its neighbours.
        int32_t drift = asleep ? 0 : (int32_t)(((app->frame / 8 + id * 3) % 4) / 2) * 2 - 1;
        digivice_draw_homage(
            canvas,
            id,
            x + drift,
            foot,
            app->frame + id * 5,
            asleep ? DigivicePoseSleep : DigivicePoseIdle,
            false,
            false,
            true);
        if(asleep) digivice_draw_zzz(canvas, x + 6, foot - 14, app->frame + id * 5);
        if(id == app->nido_sel) {
            digivice_draw_brackets(canvas, x - 9, foot - 14, x + 9, foot + 2, 3);
        }
    }

    // Panel: name, attribute, companion mark.
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 51, 128, 13);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, 51, 127, 51);
    digivice_draw_text(canvas, 3, 53, digivice_homage_name(app->nido_sel));
    const char* attr =
        digivice_attr_name(digivice_homage_attr(app->nido_sel), (DigiviceLang)s->lang);
    digivice_draw_text(canvas, 125 - digivice_draw_text_width(canvas, attr), 53, attr);
    bool chosen = s->companion == app->nido_sel;
    const char* line = digivice_app_str(app, chosen ? DigiviceStrNidoChosen : DigiviceStrNidoPick);
    digivice_draw_text(canvas, 3, 59, line);
}

void digivice_app_draw_companion(
    Canvas* canvas,
    const DigiviceApp* app,
    int32_t x,
    int32_t foot_y,
    bool asleep) {
    uint8_t id = app->state.companion;
    if(id >= DIGIVICE_HOMAGE_COUNT) return;
    digivice_draw_homage(
        canvas,
        id,
        x,
        foot_y,
        app->frame,
        asleep ? DigivicePoseSleep : DigivicePoseIdle,
        false,
        false,
        true);
}

#else

void digivice_nido_enter(DigiviceApp* app) {
    (void)app;
}

void digivice_nido_input(DigiviceApp* app, const InputEvent* event) {
    (void)event;
    digivice_app_set_screen(app, DigiviceScreenProfile);
}

void digivice_nido_draw(Canvas* canvas, DigiviceApp* app) {
    (void)canvas;
    (void)app;
}

void digivice_app_draw_companion(
    Canvas* canvas,
    const DigiviceApp* app,
    int32_t x,
    int32_t foot_y,
    bool asleep) {
    (void)canvas;
    (void)app;
    (void)x;
    (void)foot_y;
    (void)asleep;
}

#endif
