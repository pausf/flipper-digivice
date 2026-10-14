#include "digivice_app_i.h"

// Frame marks of the meal, at 10 fps: the chip drops, three bites, then hearts.
#define FEED_DROP_END 12
#define FEED_BITE_1   16
#define FEED_BITE_GAP 6
#define FEED_HEARTS   36
#define FEED_END      DIGIVICE_FEED_FRAMES

void digivice_feed_enter(DigiviceApp* app) {
    uint8_t hunger = app->state.hunger;
    if(!digivice_feed(app->digivice)) {
        digivice_app_notice(app, DigiviceStrNoticeFull);
        return;
    }
    app->feed_from = hunger;
    digivice_get_state(app->digivice, &app->state);
    digivice_app_set_screen(app, DigiviceScreenFeed);
}

static void digivice_feed_leave(DigiviceApp* app) {
    digivice_app_set_screen(app, DigiviceScreenMain);
    digivice_app_refresh(app);
}

void digivice_feed_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type == InputTypeShort && (event->key == InputKeyOk || event->key == InputKeyBack)) {
        digivice_feed_leave(app);
    }
}

void digivice_feed_tick(DigiviceApp* app) {
    if(app->screen_frame >= FEED_END) digivice_feed_leave(app);
}

void digivice_feed_draw(Canvas* canvas, DigiviceApp* app) {
    uint32_t f = app->screen_frame;
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_grid(canvas, 40, app->frame);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 0, 128, 10);
    digivice_app_draw_title(canvas, digivice_app_str(app, DigiviceStrMenuFeed));

    uint8_t bites = 0;
    if(f >= FEED_BITE_1) bites = (uint8_t)((f - FEED_BITE_1) / FEED_BITE_GAP + 1);
    if(bites > 3) bites = 3;
    bool chomp = f >= FEED_BITE_1 && f < FEED_HEARTS && ((f - FEED_BITE_1) % FEED_BITE_GAP) < 3;
    bool happy = f >= FEED_HEARTS;
    digivice_draw_form(
        canvas,
        (DigiviceForm)app->state.form,
        48,
        40,
        app->frame,
        chomp ? DigivicePoseAttack : DigivicePoseIdle,
        false);

    // The chip falls from above the screen and lands in front of the pet.
    int32_t fy = f < FEED_DROP_END ? -8 + (int32_t)f * 34 / FEED_DROP_END : 26;
    digivice_draw_food(canvas, 62, fy, bites);

    if(f >= FEED_BITE_1 && f < FEED_HEARTS) {
        for(int32_t i = 0; i < 5; i++) {
            int32_t q = (int32_t)((f * 2 + (uint32_t)i * 2) % 10);
            canvas_draw_dot(canvas, 58 + i * 3 + q * 6 / 10, 30 + q * 14 / 10);
        }
    }
    if(happy) {
        for(int32_t i = 0; i < 3; i++) {
            int32_t q = (int32_t)((f - FEED_HEARTS + (uint32_t)i * 4) % 12);
            digivice_draw_heart(canvas, 60 + i * 9, 30 - q * 18 / 12);
        }
        digivice_draw_text(canvas, 4, 54, digivice_app_str(app, DigiviceStrFeedYum));
    }

    // Hunger box: the hearts fill up to the new value once the meal is over.
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 84, 50, 42, 13);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_brackets(canvas, 84, 50, 125, 62, 2);
    digivice_draw_text(canvas, 87, 51, digivice_app_str(app, DigiviceStrFeedHunger));
    uint8_t hunger = happy ? app->state.hunger : app->feed_from;
    for(uint8_t i = 0; i < DIGIVICE_HUNGER_MAX; i++) {
        if(i < hunger) {
            digivice_draw_heart(canvas, 87 + i * 9, 57);
        } else {
            canvas_draw_box(canvas, 88 + i * 9, 60, 3, 1);
        }
    }
}
