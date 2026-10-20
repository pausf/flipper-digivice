#include "digivice_app_i.h"

void digivice_dex_enter(DigiviceApp* app) {
    app->dex_sel = (uint8_t)(app->state.form < DigiviceFormCount ? app->state.form : 0);
    digivice_app_set_screen(app, DigiviceScreenDex);
}

void digivice_dex_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return;
    switch(event->key) {
    case InputKeyRight:
        app->dex_sel = (uint8_t)((app->dex_sel + 1) % DigiviceFormCount);
        break;
    case InputKeyLeft:
        app->dex_sel = (uint8_t)((app->dex_sel + DigiviceFormCount - 1) % DigiviceFormCount);
        break;
    case InputKeyBack:
        digivice_app_set_screen(app, DigiviceScreenProfile);
        break;
    default:
        break;
    }
}

void digivice_dex_draw(Canvas* canvas, DigiviceApp* app) {
    const DigiviceRosterEntry* e = digivice_roster(app->dex_sel);
    canvas_set_color(canvas, ColorBlack);
    digivice_app_draw_title(canvas, e ? e->name : "?");
    digivice_draw_brackets(canvas, 34, 11, 94, 52, 4);
    digivice_draw_form(
        canvas, (DigiviceForm)app->dex_sel, 64, 34, app->frame, DigivicePoseIdle, false);
    if(e) {
        const char* attr =
            digivice_attr_name((DigiviceAttr)e->attr, (DigiviceLang)app->state.lang);
        digivice_draw_text(
            canvas, 3, 55, digivice_app_str(app, (DigiviceStr)(DigiviceStrStageEgg + e->stage)));
        int32_t attr_x = 125 - (int32_t)digivice_draw_text_width(canvas, attr);
        digivice_draw_text(canvas, attr_x, 55, attr);
    }
}
