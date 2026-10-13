#include "digivice_app_i.h"

#include <furi_hal.h>
#include <notification/notification_messages.h>
#include <stdio.h>

#define TAG "DigiviceApp"

static const char* const menu_labels[DigiviceMenuCount] = {"FEED", "TRAIN", "STATS", "BATTLE"};

uint8_t digivice_app_rng_pct(void) {
    return (uint8_t)(furi_hal_random_get() % 100);
}

void digivice_meter_start(DigiviceMeter* meter) {
    meter->zone_start =
        (uint8_t)(furi_hal_random_get() % (DIGIVICE_METER_SEGMENTS - DIGIVICE_METER_ZONE + 1));
    meter->cursor = 0;
    meter->direction = 1;
}

void digivice_meter_step(DigiviceMeter* meter) {
    if(meter->cursor == 0) meter->direction = 1;
    if(meter->cursor == DIGIVICE_METER_SEGMENTS - 1) meter->direction = -1;
    meter->cursor = (uint8_t)(meter->cursor + meter->direction);
}

bool digivice_meter_hit(const DigiviceMeter* meter) {
    return digivice_rules_timing_hit(meter->cursor, meter->zone_start);
}

void digivice_app_set_screen(DigiviceApp* app, DigiviceScreen screen) {
    app->screen = screen;
    app->screen_frame = 0;
}

void digivice_app_refresh(DigiviceApp* app) {
    digivice_get_state(app->digivice, &app->state);
    bool busy = app->screen == DigiviceScreenBattle || app->screen == DigiviceScreenTrain ||
                app->screen == DigiviceScreenEvolution;
    if(app->state.evo_from != DigiviceFormNone && !busy) {
        digivice_app_set_screen(app, DigiviceScreenEvolution);
    }
}

static void digivice_app_toast(DigiviceApp* app, const char* text) {
    app->toast = text;
    app->toast_frames = 15;
}

static void digivice_menu_draw(Canvas* canvas, DigiviceApp* app) {
    uint32_t now = furi_hal_rtc_get_timestamp();
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_text(canvas, 3, 2, "DIGIVICE");
    canvas_draw_line(canvas, 3, 9, 34, 9);
    digivice_draw_text(canvas, 66, 2, digivice_form_name((DigiviceForm)app->state.form));

    digivice_draw_ring(canvas, 30, 50, 15, 3);
    digivice_draw_form(
        canvas, (DigiviceForm)app->state.form, 30, 38, app->frame, DigivicePoseIdle, false);
    digivice_draw_brackets(canvas, 12, 20, 48, 52, 4);

    bool virux = digivice_rules_virux_active(&app->state, now);
    for(uint8_t i = 0; i < DigiviceMenuCount; i++) {
        int32_t y = 14 + i * 11;
        const char* label = menu_labels[i];
        if(i == DigiviceMenuBattle && !virux) label = "------";
        digivice_draw_text(canvas, 72, y, label);
        if(i == app->menu_index) {
            digivice_draw_brackets(canvas, 66, y - 2, 120, y + 7, 3);
            if(i == DigiviceMenuBattle && virux && (app->frame % 10) < 5) {
                digivice_draw_text(canvas, 104, y, "!");
            }
        }
    }

    if(app->toast_frames) {
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 0, 56, 128, 8);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_line(canvas, 0, 56, 127, 56);
        digivice_draw_text(canvas, 3, 57, app->toast);
    }

    digivice_draw_scanline(canvas, app->frame);
    if(digivice_rules_glitchy(&app->state) && (app->frame % 12) < 2) {
        digivice_draw_glitch(canvas, app->frame, 3);
    }
}

static void digivice_menu_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return;

    if(event->key == InputKeyUp) {
        app->menu_index = (app->menu_index + DigiviceMenuCount - 1) % DigiviceMenuCount;
    } else if(event->key == InputKeyDown) {
        app->menu_index = (app->menu_index + 1) % DigiviceMenuCount;
    } else if(event->key == InputKeyBack && event->type == InputTypeShort) {
        app->running = false;
    } else if(event->key == InputKeyOk && event->type == InputTypeShort) {
        switch(app->menu_index) {
        case DigiviceMenuFeed:
            digivice_app_toast(app, digivice_feed(app->digivice) ? "> HUNGER +1" : "> NOT HUNGRY");
            digivice_app_refresh(app);
            break;
        case DigiviceMenuTrain:
            digivice_train_enter(app);
            break;
        case DigiviceMenuStats:
            digivice_app_set_screen(app, DigiviceScreenStats);
            break;
        case DigiviceMenuBattle:
            if(digivice_rules_virux_active(&app->state, furi_hal_rtc_get_timestamp())) {
                digivice_battle_enter(app);
            } else {
                digivice_app_toast(app, "> NO VIRUX NEARBY");
            }
            break;
        default:
            break;
        }
    }
}

static void digivice_stats_draw(Canvas* canvas, DigiviceApp* app) {
    const DigiviceState* s = &app->state;
    char line[32];
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_brackets(canvas, 0, 0, 127, 63, 4);
    digivice_draw_text(canvas, 4, 3, digivice_form_name((DigiviceForm)s->form));
    digivice_draw_text(canvas, 70, 3, digivice_type_name(digivice_rules_player_type(s)));
    canvas_draw_line(canvas, 4, 10, 123, 10);

    snprintf(line, sizeof(line), "EXP %lu  STR %u", (unsigned long)s->exp, (unsigned)s->strength);
    digivice_draw_text(canvas, 4, 13, line);
    snprintf(line, sizeof(line), "WIN %u  LOSE %u", (unsigned)s->wins, (unsigned)s->losses);
    digivice_draw_text(canvas, 4, 21, line);
    snprintf(
        line,
        sizeof(line),
        "HUNGER %u/%u  MOOD %u/%u",
        (unsigned)s->hunger,
        (unsigned)DIGIVICE_HUNGER_MAX,
        (unsigned)s->mood,
        (unsigned)DIGIVICE_MOOD_MAX);
    digivice_draw_text(canvas, 4, 29, line);
    snprintf(line, sizeof(line), "CORRUPTION %u%%", (unsigned)s->corruption);
    digivice_draw_text(canvas, 4, 37, line);
    snprintf(
        line,
        sizeof(line),
        "NFC%u SUB%u IR%u USB%u",
        (unsigned)s->data[DigiviceTypeNfc],
        (unsigned)s->data[DigiviceTypeSubGhz],
        (unsigned)s->data[DigiviceTypeIr],
        (unsigned)s->data[DigiviceTypeUsb]);
    digivice_draw_text(canvas, 4, 47, line);
}

static void digivice_error_draw(Canvas* canvas) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_brackets(canvas, 8, 16, 119, 47, 4);
    digivice_draw_text(canvas, 18, 24, "DIGIVICE SERVICE");
    digivice_draw_text(canvas, 18, 32, "NOT RUNNING");
    digivice_draw_text(canvas, 18, 40, "> BACK TO EXIT");
}

static void digivice_app_draw_callback(Canvas* canvas, void* context) {
    DigiviceApp* app = context;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    switch(app->screen) {
    case DigiviceScreenMenu:
        digivice_menu_draw(canvas, app);
        break;
    case DigiviceScreenStats:
        digivice_stats_draw(canvas, app);
        break;
    case DigiviceScreenTrain:
        digivice_train_draw(canvas, app);
        break;
    case DigiviceScreenBattle:
        digivice_battle_draw(canvas, app);
        break;
    case DigiviceScreenEvolution:
        digivice_draw_evolution(
            canvas,
            (DigiviceForm)app->state.evo_from,
            (DigiviceForm)app->state.form,
            app->screen_frame);
        break;
    case DigiviceScreenError:
        digivice_error_draw(canvas);
        break;
    }
    furi_mutex_release(app->mutex);
}

static void digivice_app_input_callback(InputEvent* event, void* context) {
    DigiviceApp* app = context;
    furi_message_queue_put(app->input_queue, event, FuriWaitForever);
}

static void digivice_app_input(DigiviceApp* app, const InputEvent* event) {
    switch(app->screen) {
    case DigiviceScreenMenu:
        digivice_menu_input(app, event);
        break;
    case DigiviceScreenStats:
        if(event->type == InputTypeShort &&
           (event->key == InputKeyBack || event->key == InputKeyOk)) {
            digivice_app_set_screen(app, DigiviceScreenMenu);
        }
        break;
    case DigiviceScreenTrain:
        digivice_train_input(app, event);
        break;
    case DigiviceScreenBattle:
        digivice_battle_input(app, event);
        break;
    case DigiviceScreenEvolution:
        break;
    case DigiviceScreenError:
        if(event->type == InputTypeShort && event->key == InputKeyBack) app->running = false;
        break;
    }
}

static void digivice_app_tick(DigiviceApp* app) {
    app->frame++;
    app->screen_frame++;
    if(app->toast_frames) app->toast_frames--;

    switch(app->screen) {
    case DigiviceScreenMenu:
    case DigiviceScreenStats:
        if(app->frame % 10 == 0) digivice_app_refresh(app);
        break;
    case DigiviceScreenTrain:
        digivice_train_tick(app);
        break;
    case DigiviceScreenBattle:
        digivice_battle_tick(app);
        break;
    case DigiviceScreenEvolution:
        if(app->screen_frame > DIGIVICE_EVO_FRAMES) {
            digivice_ack_evolution(app->digivice);
            app->state.evo_from = DigiviceFormNone;
            digivice_app_set_screen(app, DigiviceScreenMenu);
        }
        break;
    case DigiviceScreenError:
        break;
    }
}

int32_t digivice_app(void* p) {
    UNUSED(p);
    DigiviceApp* app = malloc(sizeof(DigiviceApp));
    memset(app, 0, sizeof(DigiviceApp));
    app->running = true;
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->notification = furi_record_open(RECORD_NOTIFICATION);

    if(furi_record_exists(RECORD_DIGIVICE)) {
        app->digivice = furi_record_open(RECORD_DIGIVICE);
        digivice_app_set_screen(app, DigiviceScreenMenu);
        digivice_app_refresh(app);
    } else {
        FURI_LOG_E(TAG, "Digivice service is not running");
        digivice_app_set_screen(app, DigiviceScreenError);
    }

    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, digivice_app_draw_callback, app);
    view_port_input_callback_set(app->view_port, digivice_app_input_callback, app);
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    uint32_t frame_ticks = furi_ms_to_ticks(DIGIVICE_APP_FRAME_MS);
    uint32_t last_tick = furi_get_tick();
    while(app->running) {
        InputEvent event;
        bool got_input = furi_message_queue_get(app->input_queue, &event, frame_ticks) ==
                         FuriStatusOk;

        furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
        if(got_input) digivice_app_input(app, &event);
        if(furi_get_tick() - last_tick >= frame_ticks) {
            last_tick += frame_ticks;
            digivice_app_tick(app);
        }
        furi_mutex_release(app->mutex);
        view_port_update(app->view_port);
    }

    notification_message(app->notification, &sequence_reset_rgb);
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    if(app->digivice) furi_record_close(RECORD_DIGIVICE);
    furi_record_close(RECORD_NOTIFICATION);
    furi_message_queue_free(app->input_queue);
    furi_mutex_free(app->mutex);
    free(app);
    return 0;
}
