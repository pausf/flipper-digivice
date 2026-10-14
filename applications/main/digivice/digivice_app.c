#include "digivice_app_i.h"

#include <furi_hal.h>
#include <notification/notification_messages.h>

#define TAG "DigiviceApp"

// Middle band between the two icon bars.
#define BAND_TOP    12
#define BAND_BOTTOM 52
#define FLOOR_Y     47

uint8_t digivice_app_rng_pct(void) {
    return (uint8_t)(furi_hal_random_get() % 100);
}

const char* digivice_app_str(const DigiviceApp* app, DigiviceStr id) {
    return digivice_str((DigiviceLang)app->state.lang, id);
}

void digivice_app_notice(DigiviceApp* app, DigiviceStr id) {
    app->notice = id;
    app->notice_frames = DIGIVICE_NOTICE_FRAMES;
}

void digivice_app_refusal(DigiviceApp* app, DigiviceResult result) {
    static const DigiviceStr refusal_str[] = {
        [DigiviceResultSick] = DigiviceStrNoticeSick,
        [DigiviceResultAsleep] = DigiviceStrNoticeAsleep,
        [DigiviceResultAwake] = DigiviceStrNoticeAwake,
        [DigiviceResultHealthy] = DigiviceStrNoticeHealthy,
        [DigiviceResultNothingToClean] = DigiviceStrNoticeNothingToClean,
    };
    if(result == DigiviceResultOk || result > DigiviceResultNothingToClean) return;
    digivice_app_notice(app, refusal_str[result]);
}

void digivice_app_draw_title(Canvas* canvas, const char* title) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_text(canvas, 3, 2, title);
    canvas_draw_line(canvas, 3, 9, 3 + digivice_draw_text_width(canvas, title), 9);
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

/** Pet x on the main screen: a slow walk back and forth across the band. */
static int32_t digivice_main_pet_x(const DigiviceApp* app) {
    int32_t walk = (int32_t)((app->frame / 2) % 40);
    return 34 + (walk < 20 ? walk : 40 - walk);
}

static void digivice_main_draw(Canvas* canvas, DigiviceApp* app) {
    const DigiviceState* s = &app->state;
    uint32_t now = furi_hal_rtc_get_timestamp();
    bool asleep = digivice_rules_is_asleep(now);
    int32_t pet_x = digivice_main_pet_x(app);

    canvas_set_color(canvas, ColorBlack);
    for(int32_t x = 0; x < 128; x += 4)
        canvas_draw_dot(canvas, x, FLOOR_Y);

    // The sick pet shivers.
    int32_t shake = (s->sick && (app->frame % 2)) ? 1 : 0;
    digivice_draw_form(
        canvas,
        (DigiviceForm)s->form,
        pet_x + shake,
        38,
        app->frame,
        asleep ? DigivicePoseSleep : DigivicePoseIdle,
        false);

    digivice_draw_poops(canvas, s->poop, 96, 39, app->frame);
    if(s->sick) digivice_draw_skull(canvas, pet_x - 3, 18 + (int32_t)((app->frame / 3) % 3));

    if(asleep) {
        if(!s->light_on) digivice_draw_dark(canvas, BAND_TOP, BAND_BOTTOM, 12, app->frame);
        canvas_set_color(canvas, s->light_on ? ColorBlack : ColorWhite);
        digivice_draw_zzz(canvas, pet_x + 12, 26, app->frame);
        canvas_set_color(canvas, ColorBlack);
    }

    // Icon bars: white boxes over anything the band drew, then the two divider lines.
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 0, 128, 11);
    canvas_draw_box(canvas, 0, 53, 128, 11);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, 11, 127, 11);
    canvas_draw_line(canvas, 0, 52, 127, 52);
    for(uint8_t i = 0; i < DigiviceIconCount; i++) {
        int32_t x = 8 + (i % 4) * 31;
        int32_t y = i < 4 ? 1 : 54;
        digivice_draw_icon(canvas, (DigiviceIcon)i, x, y, i == app->menu_index);
    }
    if(digivice_rules_virux_active(s, now) && (app->frame % 10) < 5) {
        digivice_draw_text(canvas, 8 + 2 * 31 + 11, 2, "!");
    }

    if(app->notice_frames) digivice_draw_label(canvas, 14, digivice_app_str(app, app->notice));

    digivice_draw_scanline(canvas, app->frame);
    if(digivice_rules_glitchy(s) && (app->frame % 12) < 2) {
        digivice_draw_glitch(canvas, app->frame, 3);
    }
}

static void digivice_main_open(DigiviceApp* app) {
    uint32_t now = furi_hal_rtc_get_timestamp();
    DigiviceResult result;
    switch(app->menu_index) {
    case DigiviceIconFeed:
        digivice_app_notice(
            app, digivice_feed(app->digivice) ? DigiviceStrFeedYum : DigiviceStrNoticeFull);
        digivice_app_refresh(app);
        break;
    case DigiviceIconTrain:
        result = digivice_rules_can_train(&app->state, now);
        if(result == DigiviceResultOk) {
            digivice_train_enter(app);
        } else {
            digivice_app_refusal(app, result);
        }
        break;
    case DigiviceIconBattle:
        result = digivice_rules_can_battle(&app->state);
        if(result != DigiviceResultOk) {
            digivice_app_refusal(app, result);
        } else if(digivice_rules_virux_active(&app->state, now)) {
            digivice_battle_enter(app);
        } else {
            digivice_app_notice(app, DigiviceStrNoticeNoVirux);
        }
        break;
    case DigiviceIconClean:
        result = digivice_clean(app->digivice);
        digivice_app_refresh(app);
        if(result == DigiviceResultOk) {
            digivice_app_notice(app, DigiviceStrCleanDone);
        } else {
            digivice_app_refusal(app, result);
        }
        break;
    case DigiviceIconLight:
        result = digivice_toggle_light(app->digivice);
        digivice_app_refresh(app);
        if(result == DigiviceResultOk) {
            digivice_app_notice(
                app, app->state.light_on ? DigiviceStrLightOn : DigiviceStrLightNight);
        } else {
            digivice_app_refusal(app, result);
        }
        break;
    case DigiviceIconCure:
        result = digivice_cure(app->digivice);
        digivice_app_refresh(app);
        if(result == DigiviceResultOk) {
            digivice_app_notice(
                app, app->state.sick ? DigiviceStrCureMoreDose : DigiviceStrCureDone);
        } else {
            digivice_app_refusal(app, result);
        }
        break;
    case DigiviceIconProfile:
        digivice_app_set_screen(app, DigiviceScreenProfile);
        break;
    case DigiviceIconSettings:
        digivice_settings_enter(app);
        break;
    default:
        break;
    }
}

static void digivice_main_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return;

    uint8_t index = app->menu_index;
    if(event->key == InputKeyLeft) {
        index = (index + DigiviceIconCount - 1) % DigiviceIconCount;
    } else if(event->key == InputKeyRight) {
        index = (index + 1) % DigiviceIconCount;
    } else if(event->key == InputKeyUp || event->key == InputKeyDown) {
        index = (index + DigiviceIconCount / 2) % DigiviceIconCount;
    } else if(event->type == InputTypeShort && event->key == InputKeyBack) {
        app->running = false;
    } else if(event->type == InputTypeShort && event->key == InputKeyOk) {
        digivice_main_open(app);
    }
    if(index != app->menu_index) {
        app->menu_index = index;
        // The icon's name shows briefly, so every icon explains itself.
        digivice_app_notice(app, (DigiviceStr)(DigiviceStrMenuFeed + index));
    }
}

static void digivice_error_draw(Canvas* canvas, DigiviceApp* app) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_brackets(canvas, 8, 16, 119, 47, 4);
    digivice_draw_text(canvas, 18, 24, digivice_app_str(app, DigiviceStrErrorService));
    digivice_draw_text(canvas, 18, 32, digivice_app_str(app, DigiviceStrErrorNotRunning));
    digivice_draw_text(canvas, 18, 40, digivice_app_str(app, DigiviceStrErrorBack));
}

static void digivice_app_draw_callback(Canvas* canvas, void* context) {
    DigiviceApp* app = context;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    switch(app->screen) {
    case DigiviceScreenMain:
        digivice_main_draw(canvas, app);
        break;
    case DigiviceScreenTrain:
        digivice_train_draw(canvas, app);
        break;
    case DigiviceScreenBattle:
        digivice_battle_draw(canvas, app);
        break;
    case DigiviceScreenProfile:
        digivice_profile_draw(canvas, app);
        break;
    case DigiviceScreenSettings:
        digivice_settings_draw(canvas, app);
        break;
    case DigiviceScreenEvolution:
        digivice_draw_evolution(
            canvas,
            (DigiviceForm)app->state.evo_from,
            (DigiviceForm)app->state.form,
            app->screen_frame,
            0,
            (DigiviceLang)app->state.lang);
        break;
    case DigiviceScreenError:
        digivice_error_draw(canvas, app);
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
    case DigiviceScreenMain:
        digivice_main_input(app, event);
        break;
    case DigiviceScreenTrain:
        digivice_train_input(app, event);
        break;
    case DigiviceScreenBattle:
        digivice_battle_input(app, event);
        break;
    case DigiviceScreenProfile:
        if(event->type == InputTypeShort &&
           (event->key == InputKeyBack || event->key == InputKeyOk)) {
            digivice_app_set_screen(app, DigiviceScreenMain);
        }
        break;
    case DigiviceScreenSettings:
        digivice_settings_input(app, event);
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
    if(app->notice_frames) app->notice_frames--;

    switch(app->screen) {
    case DigiviceScreenMain:
    case DigiviceScreenProfile:
    case DigiviceScreenSettings:
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
            digivice_app_set_screen(app, DigiviceScreenMain);
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
        digivice_app_set_screen(app, DigiviceScreenMain);
        digivice_app_refresh(app);
        digivice_app_notice(app, DigiviceStrMenuFeed);
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
