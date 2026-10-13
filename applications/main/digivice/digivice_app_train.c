#include "digivice_app_i.h"

#define TRAIN_RESULT_FRAMES 15

void digivice_train_enter(DigiviceApp* app) {
    digivice_meter_start(&app->meter);
    app->train_done = false;
    app->train_success = false;
    digivice_app_set_screen(app, DigiviceScreenTrain);
}

static void digivice_train_finish(DigiviceApp* app, bool success) {
    app->train_done = true;
    app->train_success = success;
    app->screen_frame = 0;
    digivice_train(app->digivice, success);
    digivice_get_state(app->digivice, &app->state);
}

void digivice_train_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort || app->train_done) return;
    if(event->key == InputKeyOk) {
        digivice_train_finish(app, digivice_meter_hit(&app->meter));
    } else if(event->key == InputKeyBack) {
        // Leaving before the press costs nothing.
        digivice_app_set_screen(app, DigiviceScreenMenu);
    }
}

void digivice_train_tick(DigiviceApp* app) {
    if(app->train_done) {
        if(app->screen_frame >= TRAIN_RESULT_FRAMES) {
            digivice_app_set_screen(app, DigiviceScreenMenu);
            digivice_app_refresh(app);
        }
        return;
    }
    digivice_meter_step(&app->meter);
    if(app->screen_frame >= DIGIVICE_APP_AIM_FRAMES) digivice_train_finish(app, false);
}

void digivice_train_draw(Canvas* canvas, DigiviceApp* app) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_text(canvas, 3, 2, "TRAINING");
    canvas_draw_line(canvas, 3, 9, 34, 9);

    // A target post on the right, the pet charging at it from the left.
    canvas_draw_frame(canvas, 96, 18, 10, 26);
    canvas_draw_line(canvas, 96, 31, 105, 31);
    bool lunge = app->train_done && app->train_success && app->screen_frame < 6;
    DigivicePose pose = lunge ? DigivicePoseAttack : DigivicePoseIdle;
    digivice_draw_form(
        canvas, (DigiviceForm)app->state.form, lunge ? 64 : 40, 34, app->frame, pose, false);
    digivice_draw_brackets(canvas, 20, 14, 112, 48, 4);

    if(!app->train_done) {
        digivice_draw_meter(canvas, 14, 55, app->meter.zone_start, app->meter.cursor);
    } else {
        digivice_draw_text(
            canvas, 14, 55, app->train_success ? "> STR +1  MOOD +1" : "> MISSED...");
        if(app->train_success && app->screen_frame < 4) {
            digivice_draw_glitch(canvas, app->frame, 4);
        }
    }
}
