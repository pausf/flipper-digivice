#include "digivice_app_i.h"

// Frame marks of the meal, at 10 fps: the food drops, three bites, then hearts.
#define FEED_DROP_END   12
#define FEED_BITE_1     16
#define FEED_BITE_GAP   6
#define FEED_HEARTS     36
#define FEED_END        DIGIVICE_FEED_FRAMES
#define FEED_REFUSE_END 26

typedef enum {
    FeedPhasePick,
    FeedPhaseEat,
    FeedPhaseRefused,
} FeedPhase;

void digivice_feed_enter(DigiviceApp* app) {
    app->feed_phase = FeedPhasePick;
    app->feed_food = DigiviceFoodMeat;
    digivice_app_set_screen(app, DigiviceScreenFeed);
}

static void digivice_feed_leave(DigiviceApp* app) {
    digivice_app_set_screen(app, DigiviceScreenMain);
    digivice_app_refresh(app);
}

static uint8_t digivice_feed_level(const DigiviceApp* app, uint8_t food) {
    return food == DigiviceFoodMeat ? app->state.hunger : app->state.vigor;
}

static void digivice_feed_eat(DigiviceApp* app) {
    app->feed_from = digivice_feed_level(app, app->feed_food);
    bool ate = digivice_feed(app->digivice, (DigiviceFood)app->feed_food);
    digivice_get_state(app->digivice, &app->state);
    app->feed_phase = ate ? FeedPhaseEat : FeedPhaseRefused;
    app->screen_frame = 0;
}

void digivice_feed_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;
    if(event->key == InputKeyBack) {
        digivice_feed_leave(app);
        return;
    }
    if(app->feed_phase == FeedPhasePick) {
        if(event->key == InputKeyLeft || event->key == InputKeyRight) {
            app->feed_food = app->feed_food == DigiviceFoodMeat ? DigiviceFoodProtein :
                                                                   DigiviceFoodMeat;
        } else if(event->key == InputKeyOk) {
            digivice_feed_eat(app);
        }
    } else if(event->key == InputKeyOk) {
        digivice_feed_leave(app);
    }
}

void digivice_feed_tick(DigiviceApp* app) {
    uint32_t end = app->feed_phase == FeedPhaseEat ? FEED_END : FEED_REFUSE_END;
    if(app->feed_phase != FeedPhasePick && app->screen_frame >= end) digivice_feed_leave(app);
}

static void
    digivice_feed_hearts(Canvas* canvas, int32_t x, int32_t y, uint8_t value, uint8_t max) {
    for(uint8_t i = 0; i < max; i++) {
        if(i < value) {
            digivice_draw_heart(canvas, x + i * 9, y);
        } else {
            canvas_draw_box(canvas, x + 1 + i * 9, y + 3, 3, 1);
        }
    }
}

static void digivice_feed_draw_pick(Canvas* canvas, DigiviceApp* app) {
    const int32_t xs[DigiviceFoodCount] = {34, 92};
    const DigiviceStr names[DigiviceFoodCount] = {DigiviceStrFeedMeat, DigiviceStrFeedProtein};
    for(uint8_t i = 0; i < DigiviceFoodCount; i++) {
        digivice_draw_food_kind(canvas, (DigiviceFood)i, xs[i], 26, 0);
        const char* name = digivice_app_str(app, names[i]);
        int32_t half = (int32_t)digivice_draw_text_width(canvas, name) / 2;
        digivice_draw_text(canvas, xs[i] - half, 36, name);
        if(i == app->feed_food) {
            digivice_draw_brackets(canvas, xs[i] - 18, 13, xs[i] + 18, 43, 3);
        }
    }
    digivice_draw_text(canvas, 3, 47, digivice_app_str(app, DigiviceStrFeedHunger));
    digivice_feed_hearts(canvas, 44, 47, app->state.hunger, DIGIVICE_HUNGER_MAX);
    digivice_draw_text(canvas, 3, 55, digivice_app_str(app, DigiviceStrFeedVigor));
    digivice_feed_hearts(canvas, 44, 55, app->state.vigor, DIGIVICE_VIGOR_MAX);
}

void digivice_feed_draw(Canvas* canvas, DigiviceApp* app) {
    uint32_t f = app->screen_frame;
    canvas_set_color(canvas, ColorBlack);
    digivice_app_draw_title(canvas, digivice_app_str(app, DigiviceStrMenuFeed));
    if(app->feed_phase == FeedPhasePick) {
        digivice_feed_draw_pick(canvas, app);
        return;
    }

    digivice_draw_grid(canvas, 40, app->frame);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 0, 128, 10);
    digivice_app_draw_title(canvas, digivice_app_str(app, DigiviceStrMenuFeed));

    DigiviceFood food = (DigiviceFood)app->feed_food;
    if(app->feed_phase == FeedPhaseRefused) {
        // Head-shake: the pet jitters left and right while the untouched food waits.
        int32_t dx = f < FEED_REFUSE_END - 6 ? ((f / 2) % 2 ? -3 : 3) : 0;
        digivice_draw_form(
            canvas,
            (DigiviceForm)app->state.form,
            48 + dx,
            40,
            app->frame,
            DigivicePoseIdle,
            false);
        digivice_draw_food_kind(canvas, food, 62, 26, 0);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 0, 52, 128, 12);
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_text(canvas, 4, 55, digivice_app_str(app, DigiviceStrNoticeFull));
        return;
    }

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

    // The food falls from above the screen and lands in front of the pet.
    int32_t fy = f < FEED_DROP_END ? -8 + (int32_t)f * 34 / FEED_DROP_END : 26;
    digivice_draw_food_kind(canvas, food, 62, fy, bites);
    if(food == DigiviceFoodProtein && f >= FEED_DROP_END && f < FEED_DROP_END + 4) {
        digivice_draw_sparkle(canvas, 70, 18, 2);
        digivice_draw_text(canvas, 66, 10, "CLINK");
    }

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

    // Meter box: the hearts fill up to the new value once the meal is over.
    bool meat = food == DigiviceFoodMeat;
    uint8_t max = meat ? DIGIVICE_HUNGER_MAX : DIGIVICE_VIGOR_MAX;
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 84, 50, 42, 13);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_brackets(canvas, 84, 50, 125, 62, 2);
    digivice_draw_text(
        canvas, 87, 51, digivice_app_str(app, meat ? DigiviceStrFeedHunger : DigiviceStrFeedVigor));
    uint8_t level = happy ? digivice_feed_level(app, app->feed_food) : app->feed_from;
    digivice_feed_hearts(canvas, 87, 57, level, max);
}
