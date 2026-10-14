#include "digivice_app_i.h"

static const DigiviceStr setting_labels[DigiviceSettingCount] = {
    DigiviceStrSettingsLanguage,
    DigiviceStrSettingsMode,
    DigiviceStrSettingsReset,
};

void digivice_settings_enter(DigiviceApp* app) {
    app->setting = DigiviceSettingLanguage;
    app->confirm = 0;
    digivice_app_set_screen(app, DigiviceScreenSettings);
}

static void digivice_settings_change(DigiviceApp* app) {
    if(app->setting == DigiviceSettingLanguage) {
        uint8_t lang = (app->state.lang + 1) % DigiviceLangCount;
        digivice_set_lang(app->digivice, (DigiviceLang)lang);
    } else if(app->setting == DigiviceSettingMode) {
        uint8_t mode = (app->state.mode + 1) % DigiviceModeCount;
        digivice_set_mode(app->digivice, (DigiviceMode)mode);
    }
    digivice_get_state(app->digivice, &app->state);
}

void digivice_settings_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;

    if(app->confirm) {
        // The reset needs OK twice; any other key backs out.
        if(event->key == InputKeyOk && app->confirm == 1) {
            app->confirm = 2;
        } else if(event->key == InputKeyOk) {
            digivice_reset(app->digivice);
            app->confirm = 0;
            app->menu_index = DigiviceIconFeed;
            digivice_app_set_screen(app, DigiviceScreenMain);
            digivice_app_refresh(app);
            digivice_app_notice(app, DigiviceStrResetDone);
        } else {
            app->confirm = 0;
        }
        return;
    }

    switch(event->key) {
    case InputKeyUp:
        app->setting = (app->setting + DigiviceSettingCount - 1) % DigiviceSettingCount;
        break;
    case InputKeyDown:
        app->setting = (app->setting + 1) % DigiviceSettingCount;
        break;
    case InputKeyLeft:
    case InputKeyRight:
        digivice_settings_change(app);
        break;
    case InputKeyOk:
        if(app->setting == DigiviceSettingReset) {
            app->confirm = 1;
        } else {
            digivice_settings_change(app);
        }
        break;
    case InputKeyBack:
        digivice_app_set_screen(app, DigiviceScreenMain);
        break;
    default:
        break;
    }
}

static const char* digivice_settings_value(DigiviceApp* app, uint8_t setting) {
    if(setting == DigiviceSettingLanguage) {
        // Each language names itself, whatever the current one is.
        return digivice_str((DigiviceLang)app->state.lang, DigiviceStrLangName);
    }
    if(setting == DigiviceSettingMode) {
        return digivice_app_str(
            app,
            app->state.mode == DigiviceModeHacker ? DigiviceStrModeHacker :
                                                    DigiviceStrModeNormal);
    }
    return NULL;
}

void digivice_settings_draw(Canvas* canvas, DigiviceApp* app) {
    canvas_set_color(canvas, ColorBlack);
    digivice_app_draw_title(canvas, digivice_app_str(app, DigiviceStrMenuSettings));

    for(uint8_t i = 0; i < DigiviceSettingCount; i++) {
        int32_t y = 17 + i * 13;
        digivice_draw_text(canvas, 8, y, digivice_app_str(app, setting_labels[i]));
        const char* value = digivice_settings_value(app, i);
        if(value) {
            digivice_draw_text(canvas, 120 - digivice_draw_text_width(canvas, value), y, value);
        }
        if(i == app->setting) digivice_draw_brackets(canvas, 3, y - 3, 124, y + 8, 3);
    }

    if(app->confirm) {
        DigiviceStr question = app->confirm == 1 ? DigiviceStrConfirmSure :
                                                   DigiviceStrConfirmReally;
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 10, 20, 108, 26);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, 10, 20, 108, 26);
        const char* q = digivice_app_str(app, question);
        digivice_draw_text(canvas, 64 - digivice_draw_text_width(canvas, q) / 2, 25, q);
        const char* hint = digivice_app_str(app, DigiviceStrConfirmHint);
        digivice_draw_text(canvas, 64 - digivice_draw_text_width(canvas, hint) / 2, 35, hint);
    }
}
