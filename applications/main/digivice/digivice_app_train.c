#include "digivice_app_i.h"

#include <stdio.h>

#define TRAIN_RESULT_FRAMES 15

void digivice_train_enter(DigiviceApp* app) {
    digivice_meter_start(&app->meter);
    app->train_done = false;
    app->train_success = false;
    digivice_app_set_screen(app, DigiviceScreenTrain);
}

static void digivice_train_finish(DigiviceApp* app, bool success) {
    DigiviceResult result = digivice_train(app->digivice, success);
    digivice_get_state(app->digivice, &app->state);
    if(result != DigiviceResultOk) {
        // The sleep window can start while the meter runs: say why nothing happened.
        digivice_app_set_screen(app, DigiviceScreenMain);
        digivice_app_refusal(app, result);
        return;
    }
    app->train_done = true;
    app->train_success = success;
    app->screen_frame = 0;
}

void digivice_train_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort || app->train_done) return;
    if(event->key == InputKeyOk) {
        digivice_train_finish(app, digivice_meter_hit(&app->meter));
    } else if(event->key == InputKeyBack) {
        // Leaving before the press costs nothing.
        digivice_app_set_screen(app, DigiviceScreenMain);
    }
}

void digivice_train_tick(DigiviceApp* app) {
    if(app->train_done) {
        if(app->screen_frame >= TRAIN_RESULT_FRAMES) {
            digivice_app_set_screen(app, DigiviceScreenMain);
            digivice_app_refresh(app);
        }
        return;
    }
    digivice_meter_step(&app->meter);
    if(app->screen_frame >= DIGIVICE_APP_AIM_FRAMES) digivice_train_finish(app, false);
}

void digivice_train_draw(Canvas* canvas, DigiviceApp* app) {
    canvas_set_color(canvas, ColorBlack);
    digivice_app_draw_title(canvas, digivice_app_str(app, DigiviceStrMenuTrain));
    const char* hint = digivice_app_str(app, DigiviceStrTrainHint);
    digivice_draw_text(canvas, 125 - digivice_draw_text_width(canvas, hint), 2, hint);

    // A target post on the right, the pet charging at it from the left.
    canvas_draw_frame(canvas, 96, 18, 10, 26);
    canvas_draw_line(canvas, 96, 31, 105, 31);
    bool lunge = app->train_done && app->train_success && app->screen_frame < 6;
    DigivicePose pose = lunge ? DigivicePoseAttack : DigivicePoseIdle;
    digivice_draw_form(
        canvas, (DigiviceForm)app->state.form, lunge ? 64 : 40, 34, app->frame, pose, false);
    digivice_draw_brackets(canvas, 20, 14, 112, 47, 4);

    if(!app->train_done) {
        digivice_draw_meter(canvas, 14, 55, app->meter.zone_start, app->meter.cursor);
        return;
    }
    digivice_draw_text(canvas, 4, 49, ">");
    digivice_draw_text(
        canvas,
        9,
        49,
        digivice_app_str(app, app->train_success ? DigiviceStrTrainHit : DigiviceStrTrainMiss));
    // Label (3 glyphs, 6 bytes at most) + " +" + a uint8_t + NUL.
    char line[16];
    uint8_t gain = app->train_success ? DIGIVICE_TRAIN_EXP_OK : DIGIVICE_TRAIN_EXP_FAIL;
    snprintf(line, sizeof(line), "%s +%u", digivice_app_str(app, DigiviceStrExp), (unsigned)gain);
    digivice_draw_text(canvas, 9, 57, line);
    if(app->train_success && app->screen_frame < 4) {
        digivice_draw_glitch(canvas, app->frame, 4);
    }
}
