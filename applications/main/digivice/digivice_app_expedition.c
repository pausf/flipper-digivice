#include "digivice_app_i.h"
#include <digivice/digivice_notify.h>
#include <furi_hal.h>
#include <notification/notification_messages.h>

#include <stdio.h>

#define TAG "DigiviceTrail"

// Progress line of the trail view (expedicion.html, trail scene).
#define LINE_X0 8
#define LINE_X1 116
#define LINE_Y  30

#define HIKE_TARGET_X 60
#define HIKE_FLASH    2

// Map nodes on a zig-zag, one per area.
static const uint8_t node_x[DIGIVICE_AREA_COUNT] = {10, 25, 41, 56, 72, 88, 104, 118};
static const uint8_t node_y[DIGIVICE_AREA_COUNT] = {44, 25, 44, 23, 43, 22, 40, 20};

static const DigiviceStr next_str[DigiviceMidwayCount] = {
    DigiviceStrExpNextSpring,
    DigiviceStrExpNextCache,
    DigiviceStrExpNextShortcut,
};

static const DigiviceStr mid_title[DigiviceMidwayCount] = {
    DigiviceStrMidTitleSpring,
    DigiviceStrMidTitleCache,
    DigiviceStrMidTitleShortcut,
};

static bool trail_walking(const DigiviceApp* app, uint32_t now) {
    return digivice_expedition_can_walk(&app->state) && !digivice_rules_is_asleep(now);
}

// Why the pet is not walking, or 0 (DigiviceStrMenuFeed) when it is.
static DigiviceStr idle_reason(const DigiviceApp* app, uint32_t now) {
    if(app->state.sick) return DigiviceStrExpIdleSick;
    if(digivice_rules_is_asleep(now)) return DigiviceStrExpIdleAsleep;
    if(digivice_rules_stage(&app->state) < DigiviceStageBaby) return DigiviceStrExpIdleYoung;
    return DigiviceStrMenuFeed;
}

void digivice_expedition_enter(DigiviceApp* app) {
    digivice_get_state(app->digivice, &app->state);
    notification_message(app->notification, &sequence_reset_rgb);
    digivice_app_set_screen(app, DigiviceScreenExpedition);
    digivice_app_refresh(app);
}

void digivice_expedition_auto_start(DigiviceApp* app) {
    const DigiviceState* s = &app->state;
    uint32_t now = furi_hal_rtc_get_timestamp();
    if(app->state.evo_from != DigiviceFormNone) return;
    if(digivice_rules_can_battle(s, now) != DigiviceResultOk) return;
    // Pending trail or VIRUX first, then an open boss; midway and gated bosses never auto-start.
    if(digivice_expedition_trail_pending(s, now)) {
        if(app->offered_kind == DigiviceEncounterTrail && app->offered_expires == s->enc_expires)
            return;
        digivice_encounter_start(app, DigiviceEncounterTrail);
    } else if(digivice_rules_virux_active(s, now)) {
        if(app->offered_kind == DigiviceEncounterVirux && app->offered_expires == s->virux_expires)
            return;
        digivice_encounter_start(app, DigiviceEncounterVirux);
    } else if(digivice_expedition_boss_ready(s) && digivice_expedition_boss_gate_open(s)) {
        if(app->offered_kind == DigiviceEncounterBoss && app->offered_expires == s->cleared)
            return;
        digivice_encounter_start(app, DigiviceEncounterBoss);
    }
}

static void hike_enter(DigiviceApp* app) {
    uint32_t now = furi_hal_rtc_get_timestamp();
    DigiviceResult result = digivice_rules_can_train(&app->state, now);
    if(result == DigiviceResultOk && digivice_rules_stage(&app->state) < DigiviceStageBaby) {
        result = DigiviceResultTooYoung;
    }
    if(result != DigiviceResultOk) {
        digivice_app_refusal(app, result);
        return;
    }
    if(!digivice_expedition_can_walk(&app->state)) return;
    app->hike_left = digivice_expedition_hike_left(&app->state, now);
    app->hike_good = 0;
    app->hike_paid = 0;
    app->hike_granted = 0;
    app->hike_flash = 0;
    // A capped hour shows the limit and starts no session.
    app->hike_done = app->hike_left == 0;
    app->hike_start = furi_get_tick();
    digivice_app_set_screen(app, DigiviceScreenHike);
}

static uint32_t hike_elapsed_ms(const DigiviceApp* app) {
    return (furi_get_tick() - app->hike_start) * 1000UL / furi_kernel_get_tick_frequency();
}

static void hike_finish(DigiviceApp* app) {
    app->hike_granted = digivice_hike_finish(app->digivice, app->hike_good);
    digivice_get_state(app->digivice, &app->state);
    app->hike_done = true;
}

static void midway_enter(DigiviceApp* app) {
    app->midway.kind = digivice_expedition_midway_kind(app->state.area);
    app->midway.ok = false;
    app->midway_claimed = false;
    app->midway_accepted = true;
    digivice_app_set_screen(app, DigiviceScreenMidway);
    // Spring and cache pay out at once; only the shortcut asks first.
    if(app->midway.kind != DigiviceMidwayShortcut) {
        app->midway = digivice_midway_claim(app->digivice, true);
        app->midway_claimed = true;
        digivice_get_state(app->digivice, &app->state);
    }
}

static void trail_action(DigiviceApp* app) {
    const DigiviceState* s = &app->state;
    uint32_t now = furi_hal_rtc_get_timestamp();
    if(digivice_expedition_trail_pending(s, now)) {
        digivice_encounter_start(app, DigiviceEncounterTrail);
    } else if(digivice_rules_virux_active(s, now)) {
        digivice_encounter_start(app, DigiviceEncounterVirux);
    } else if(digivice_expedition_boss_ready(s)) {
        if(digivice_expedition_boss_gate_open(s)) {
            digivice_encounter_start(app, DigiviceEncounterBoss);
        } else {
            digivice_app_set_screen(app, DigiviceScreenGate);
        }
    } else if(s->mid_state == 1) {
        midway_enter(app);
    } else if(!digivice_expedition_all_clear(s)) {
        hike_enter(app);
    }
}

static void trail_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;
    if(event->key == InputKeyOk) {
        trail_action(app);
    } else if(event->key == InputKeyUp) {
        app->map_sel = app->state.area;
        digivice_app_set_screen(app, DigiviceScreenMap);
    } else if(event->key == InputKeyBack) {
        digivice_app_set_screen(app, DigiviceScreenMain);
    }
}

static void map_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return;
    if(event->key == InputKeyLeft && app->map_sel > 0) {
        app->map_sel--;
    } else if(event->key == InputKeyRight && app->map_sel < DIGIVICE_AREA_COUNT - 1) {
        app->map_sel++;
    } else if(event->type == InputTypeShort && event->key == InputKeyOk) {
        if(digivice_select_area(app->digivice, app->map_sel)) {
            digivice_get_state(app->digivice, &app->state);
            digivice_app_set_screen(app, DigiviceScreenExpedition);
        } else {
            digivice_app_notice(app, DigiviceStrNoticeAreaLocked);
        }
    } else if(event->type == InputTypeShort && event->key == InputKeyBack) {
        digivice_app_set_screen(app, DigiviceScreenExpedition);
    }
}

static void hike_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;
    if(app->hike_done) {
        if(event->key == InputKeyOk || event->key == InputKeyBack) digivice_expedition_enter(app);
        return;
    }
    if(event->key == InputKeyBack) {
        hike_finish(app);
    } else if(event->key == InputKeyOk) {
        uint8_t beat = 0;
        int32_t phase = digivice_expedition_hike_phase(hike_elapsed_ms(app), &beat);
        // Each beat pays once; a bad or extra tap does nothing.
        if(digivice_expedition_hike_tap_good(phase) && !(app->hike_paid & (1UL << beat))) {
            app->hike_paid |= 1UL << beat;
            app->hike_good++;
            app->hike_flash = HIKE_FLASH;
            notification_message(app->notification, &sequence_blink_green_100);
        }
    }
}

static void midway_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;
    if(app->midway_claimed) {
        if(event->key == InputKeyOk || event->key == InputKeyBack) digivice_expedition_enter(app);
        return;
    }
    if(event->key == InputKeyOk || event->key == InputKeyBack) {
        app->midway_accepted = event->key == InputKeyOk;
        app->midway = digivice_midway_claim(app->digivice, app->midway_accepted);
        app->midway_claimed = true;
        digivice_get_state(app->digivice, &app->state);
    }
}

static void gate_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;
    if(event->key == InputKeyLeft || event->key == InputKeyBack) {
        digivice_app_set_screen(app, DigiviceScreenMain);
    } else if(event->key == InputKeyRight) {
        uint32_t now = furi_hal_rtc_get_timestamp();
        DigiviceResult result = digivice_rules_can_train(&app->state, now);
        if(result == DigiviceResultOk) {
            digivice_train_enter(app);
        } else {
            digivice_app_refusal(app, result);
        }
    }
}

void digivice_expedition_input(DigiviceApp* app, const InputEvent* event) {
    switch(app->screen) {
    case DigiviceScreenExpedition:
        trail_input(app, event);
        break;
    case DigiviceScreenMap:
        map_input(app, event);
        break;
    case DigiviceScreenHike:
        hike_input(app, event);
        break;
    case DigiviceScreenMidway:
        midway_input(app, event);
        break;
    case DigiviceScreenGate:
        gate_input(app, event);
        break;
    default:
        break;
    }
}

void digivice_expedition_tick(DigiviceApp* app) {
    uint32_t now = furi_hal_rtc_get_timestamp();
    // Distance is bookkeeping and never published: poll the service once per second.
    if(app->screen != DigiviceScreenHike && app->frame % 10 == 0) digivice_app_refresh(app);
    if(app->screen == DigiviceScreenExpedition || app->screen == DigiviceScreenHike) {
        if(app->screen == DigiviceScreenHike || trail_walking(app, now)) app->trail_scroll++;
    }
    if(app->screen != DigiviceScreenHike || app->hike_done) return;
    if(app->hike_flash) app->hike_flash--;
    uint32_t end_ms = DIGIVICE_HIKE_BEATS * DIGIVICE_HIKE_BEAT_MS + DIGIVICE_HIKE_BEAT_MS;
    if(hike_elapsed_ms(app) >= end_ms) hike_finish(app);
}

static void draw_top_bar(Canvas* canvas, const char* left, const char* right) {
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, 128, 9);
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_text(canvas, 2, 2, left);
    if(right) digivice_draw_text(canvas, 126 - digivice_draw_text_width(canvas, right), 2, right);
    canvas_set_color(canvas, ColorBlack);
}

static void draw_panel(Canvas* canvas) {
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 51, 128, 13);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, 51, 127, 51);
}

static const char* area_name(const DigiviceApp* app, uint8_t area) {
    return digivice_app_str(app, (DigiviceStr)(DigiviceStrArea1 + area));
}

static void draw_trail(Canvas* canvas, DigiviceApp* app) {
    const DigiviceState* s = &app->state;
    uint32_t now = furi_hal_rtc_get_timestamp();
    bool walking = trail_walking(app, now);
    uint16_t len = digivice_rules_area_length(s->area);
    uint16_t done = s->distance < len ? len - s->distance : 0;
    // "D" plus 5 digits for any uint16_t, then the NUL.
    char text[24];

    uint32_t start = furi_get_tick();
    digivice_draw_scene(
        canvas, digivice_app_scene(app), app->trail_scroll, app->frame, digivice_app_day_minute());
    uint32_t draw_ms = (furi_get_tick() - start) * 1000UL / furi_kernel_get_tick_frequency();
    // Device check: the scene must stay under 30 ms per 100 ms frame.
    if(app->frame % 50 == 0) FURI_LOG_I(TAG, "Scene drawn in %lu ms", (unsigned long)draw_ms);

    snprintf(text, sizeof(text), "D%04u", (unsigned)s->distance);
    draw_top_bar(canvas, area_name(app, s->area), text);
    if(s->item == DigiviceItemShield) digivice_draw_shield(canvas, 92, 0);
    if(digivice_rules_injured(s, now)) digivice_draw_bandage(canvas, 82, 1);

    // Progress strip: start post, midway flag, skull at the end, the pet marker walking.
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 18, 128, 16);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, LINE_X0, LINE_Y, LINE_X1, LINE_Y);
    for(int32_t x = LINE_X0; x < LINE_X1; x += 2)
        canvas_draw_dot(canvas, x, LINE_Y + 2);
    int32_t px = LINE_X0 + (LINE_X1 - LINE_X0) * done / (len ? len : 1);
    canvas_draw_line(canvas, LINE_X0, LINE_Y + 1, px, LINE_Y + 1);
    canvas_draw_box(canvas, LINE_X0 - 2, 22, 1, 9);
    canvas_draw_box(canvas, LINE_X0 - 1, 22, 4, 3);
    int32_t mx = (LINE_X0 + LINE_X1) / 2;
    canvas_draw_line(canvas, mx, 21, mx, LINE_Y);
    if(s->mid_state == 0) {
        canvas_draw_frame(canvas, mx + 1, 21, 5, 4);
    } else {
        canvas_draw_box(canvas, mx + 1, 21, 5, 4);
    }
    digivice_draw_skull(canvas, LINE_X1 + 2, 24);
    int32_t bob = walking && (app->frame / 2) % 2 ? 1 : 0;
    digivice_draw_ring(canvas, px, 25 - bob, 3, 3);
    digivice_draw_fill_triangle(canvas, px - 2, 22 - bob, px, 22 - bob, px - 2, 19 - bob);
    digivice_draw_fill_triangle(canvas, px + 1, 22 - bob, px + 3, 22 - bob, px + 3, 19 - bob);

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 34, 128, 17);
    canvas_set_color(canvas, ColorBlack);
    snprintf(
        text,
        sizeof(text),
        "%s %u/%u",
        digivice_app_str(app, DigiviceStrExpPaces),
        (unsigned)done,
        (unsigned)len);
    digivice_draw_text(canvas, 4, 37, text);
    snprintf(
        text,
        sizeof(text),
        "%s %u",
        digivice_app_str(app, DigiviceStrExpLeft),
        (unsigned)s->distance);
    digivice_draw_text(canvas, 4, 44, text);
    if(walking) {
        digivice_draw_text(canvas, 78, 37, digivice_app_str(app, DigiviceStrExpRate));
        digivice_draw_text(canvas, 78, 44, digivice_app_str(app, DigiviceStrExpAwake));
    }

    draw_panel(canvas);
    DigiviceStr next = DigiviceStrExpNextBoss;
    if(digivice_expedition_all_clear(s)) {
        next = DigiviceStrExpAllClear;
    } else if(s->mid_state == 0) {
        next = next_str[digivice_expedition_midway_kind(s->area)];
    }
    digivice_draw_text(canvas, 3, 53, digivice_app_str(app, next));

    DigiviceStr hint = DigiviceStrExpHintIdle;
    DigiviceStr reason = idle_reason(app, now);
    if(digivice_expedition_trail_pending(s, now) || digivice_rules_virux_active(s, now)) {
        hint = DigiviceStrExpHintFight;
    } else if(digivice_expedition_boss_ready(s)) {
        hint = digivice_expedition_boss_gate_open(s) ? DigiviceStrExpHintBoss :
                                                       DigiviceStrExpHintGate;
    } else if(s->mid_state == 1) {
        hint = DigiviceStrExpHintMidway;
    } else if(reason != DigiviceStrMenuFeed) {
        hint = reason;
    }
    digivice_draw_text(canvas, 3, 59, digivice_app_str(app, hint));
}

static void draw_map(Canvas* canvas, DigiviceApp* app) {
    const DigiviceState* s = &app->state;
    char text[40];
    draw_top_bar(canvas, digivice_app_str(app, DigiviceStrMapTitle), NULL);
    canvas_set_color(canvas, ColorBlack);
    for(uint8_t i = 0; i + 1 < DIGIVICE_AREA_COUNT; i++) {
        // Dotted path between the nodes.
        for(int32_t k = 1; k < 8; k += 2) {
            int32_t x = node_x[i] + (node_x[i + 1] - node_x[i]) * k / 8;
            int32_t y = node_y[i] + (node_y[i + 1] - node_y[i]) * k / 8;
            canvas_draw_dot(canvas, x, y);
        }
    }
    for(uint8_t i = 0; i < DIGIVICE_AREA_COUNT; i++) {
        int32_t x = node_x[i];
        int32_t y = node_y[i];
        bool done = s->cleared & (1U << i);
        bool here = s->area == i;
        bool open = digivice_expedition_area_unlocked(s, i);
        if(here && (app->frame % 6) < 2) {
            // The current node blinks.
        } else if(done) {
            digivice_draw_fill_ellipse(canvas, x, y, 4, 4);
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_line(canvas, x - 2, y, x - 1, y + 1);
            canvas_draw_line(canvas, x - 1, y + 1, x + 2, y - 2);
            canvas_set_color(canvas, ColorBlack);
        } else {
            digivice_draw_ring(canvas, x, y, 4, 4);
            if(!open) digivice_draw_text(canvas, x - 1, y - 2, "?");
            if(here) digivice_draw_fill_ellipse(canvas, x, y, 1, 1);
        }
        if(digivice_expedition_streak_area(i)) {
            digivice_draw_brackets(canvas, x - 6, y - 6, x + 6, y + 6, 2);
        }
    }
    int32_t sx = node_x[app->map_sel];
    int32_t sy = node_y[app->map_sel];
    digivice_draw_brackets(canvas, sx - 8, sy - 8, sx + 8, sy + 7, 3);

    draw_panel(canvas);
    uint8_t a = app->map_sel;
    snprintf(text, sizeof(text), "%u %s", (unsigned)(a + 1), area_name(app, a));
    digivice_draw_text(canvas, 3, 53, text);
    DigiviceStr status = s->area == a                             ? DigiviceStrMapHere :
                         (s->cleared & (1U << a))                 ? DigiviceStrMapDone :
                         !digivice_expedition_area_unlocked(s, a) ? DigiviceStrMapLocked :
                                                                    DigiviceStrMenuFeed;
    if(status != DigiviceStrMenuFeed) {
        const char* label = digivice_app_str(app, status);
        digivice_draw_text(canvas, 126 - digivice_draw_text_width(canvas, label), 53, label);
    }
    snprintf(
        text,
        sizeof(text),
        "%u %s",
        (unsigned)digivice_rules_area_length(a),
        digivice_app_str(app, DigiviceStrExpPaces));
    digivice_draw_text(canvas, 3, 59, text);
    const char* rule = digivice_app_str(
        app, digivice_expedition_streak_area(a) ? DigiviceStrMapStreak : DigiviceStrMapRetreatOk);
    digivice_draw_text(canvas, 126 - digivice_draw_text_width(canvas, rule), 59, rule);
    if(app->notice_frames) digivice_draw_label(canvas, 12, digivice_app_str(app, app->notice));
}

static void draw_hike(Canvas* canvas, DigiviceApp* app) {
    char text[24];
    snprintf(
        text,
        sizeof(text),
        "%u/%u",
        (unsigned)app->hike_good,
        (unsigned)DIGIVICE_HIKE_BEATS);
    digivice_draw_scene(
        canvas, digivice_app_scene(app), app->trail_scroll, app->frame, digivice_app_day_minute());
    draw_top_bar(canvas, digivice_app_str(app, DigiviceStrHikeTitle), text);
    digivice_draw_form(
        canvas,
        (DigiviceForm)app->state.form,
        64,
        34 - (int32_t)((app->frame / 2) % 2),
        app->frame,
        DigivicePoseIdle,
        false);
    digivice_app_draw_companion(canvas, app, 40, 46, false);

    draw_panel(canvas);
    if(app->hike_done) {
        if(app->hike_left == 0) {
            digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrHikeCapped));
        } else {
            snprintf(
                text,
                sizeof(text),
                "%s %u",
                digivice_app_str(app, DigiviceStrHikeGained),
                (unsigned)app->hike_granted);
            digivice_draw_text(canvas, 3, 53, text);
        }
        return;
    }
    digivice_draw_text(canvas, 3, 52, digivice_app_str(app, DigiviceStrHikeHint));
    snprintf(text, sizeof(text), "%u/%u", (unsigned)app->hike_left, (unsigned)DIGIVICE_HIKE_CAP);
    digivice_draw_text(canvas, 126 - digivice_draw_text_width(canvas, text), 52, text);
    // Notes scroll left 4 px per 100 ms frame towards the target box.
    int32_t elapsed = (int32_t)hike_elapsed_ms(app);
    for(int32_t k = 0; k < DIGIVICE_HIKE_BEATS; k++) {
        int32_t beat_ms = (k + 1) * DIGIVICE_HIKE_BEAT_MS;
        int32_t x = HIKE_TARGET_X + 2 + (beat_ms - elapsed) / 25;
        bool paid = app->hike_paid & (1UL << k);
        if(x >= 0 && x < 128 && !paid) canvas_draw_box(canvas, x, 58, 2, 4);
    }
    canvas_draw_frame(canvas, HIKE_TARGET_X, 57, 6, 6);
    if(app->hike_flash) {
        canvas_draw_box(canvas, HIKE_TARGET_X + 1, 58, 4, 4);
        digivice_draw_text(canvas, HIKE_TARGET_X + 10, 58, "+1");
    }
}

static void draw_midway(Canvas* canvas, DigiviceApp* app) {
    char text[24];
    uint8_t kind = app->midway.kind < DigiviceMidwayCount ? app->midway.kind : 0;
    draw_top_bar(canvas, digivice_app_str(app, mid_title[kind]), NULL);
    canvas_set_color(canvas, ColorBlack);
    if(kind == DigiviceMidwaySpring) {
        // A spring with droplets rising and falling.
        digivice_draw_ring(canvas, 92, 42, 14, 4);
        for(int32_t i = 0; i < 4; i++) {
            int32_t q = (int32_t)((app->frame + (uint32_t)i * 3) % 12);
            canvas_draw_dot(canvas, 86 + i * 4, 38 - (q < 6 ? q : 12 - q) * 2);
        }
    } else if(kind == DigiviceMidwayCache) {
        canvas_draw_frame(canvas, 82, 30, 20, 14);
        canvas_draw_line(canvas, 82, 35, 101, 35);
        canvas_draw_box(canvas, 90, 33, 4, 4);
        if(app->midway_claimed && app->midway.ok) digivice_draw_shield(canvas, 104, 20);
    } else {
        // A fork in the path.
        canvas_draw_line(canvas, 70, 46, 90, 30);
        canvas_draw_line(canvas, 90, 30, 118, 22);
        canvas_draw_line(canvas, 90, 30, 116, 44);
    }
    digivice_draw_form(
        canvas, (DigiviceForm)app->state.form, 40, 34, app->frame, DigivicePoseIdle, false);

    draw_panel(canvas);
    if(!app->midway_claimed) {
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrMidShortcutAsk));
        digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrMidShortcutKeys));
        return;
    }
    DigiviceStr line = DigiviceStrMidSkipped;
    if(kind == DigiviceMidwaySpring) {
        line = DigiviceStrMidSpring;
    } else if(kind == DigiviceMidwayCache) {
        line = app->midway.ok ? DigiviceStrMidCacheShield : DigiviceStrMidCacheExp;
    } else if(app->midway.kind == DigiviceMidwayShortcut) {
        // Skip and lose both leave ok false: midway_accepted tells them apart.
        line = !app->midway_accepted ? DigiviceStrMidSkipped :
               app->midway.ok        ? DigiviceStrMidShortcutWin :
                                       DigiviceStrMidShortcutLose;
    }
    digivice_draw_text(canvas, 3, 53, digivice_app_str(app, line));
    if(kind == DigiviceMidwayCache && !app->midway.ok) {
        snprintf(
            text,
            sizeof(text),
            "+%u %s",
            (unsigned)DIGIVICE_CACHE_EXP,
            digivice_app_str(app, DigiviceStrExp));
        digivice_draw_text(canvas, 3, 59, text);
    }
}

static void draw_gate(Canvas* canvas, DigiviceApp* app) {
    // Bosses reach 64 px wide: the pet stays left of x 64 and the boss fills the right half.
    digivice_draw_boss(
        canvas, app->state.area, (DigiviceForm)app->state.form, 96, 50, app->frame, true);
    digivice_draw_form(
        canvas, (DigiviceForm)app->state.form, 30, 39, app->frame, DigivicePoseIdle, false);
    // The bar stops at x 62 so the tallest bosses (x 64..127) are never covered.
    char title[24];
    snprintf(
        title,
        sizeof(title),
        "%s %s",
        digivice_app_str(app, DigiviceStrEncBoss),
        digivice_expedition_boss_name(app->state.area));
    canvas_draw_box(canvas, 0, 0, 62, 9);
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_text(canvas, 2, 2, title);
    canvas_set_color(canvas, ColorBlack);
    draw_panel(canvas);
    // The scratch line is wider than the pet's half: it takes turns with the requirement.
    DigiviceStr need = (app->frame / 20) % 2 ? DigiviceStrGateScratch : DigiviceStrGateNeed;
    digivice_draw_text(canvas, 3, 53, digivice_app_str(app, need));
    digivice_draw_text(canvas, 3, 59, "<");
    digivice_draw_text(canvas, 9, 59, digivice_app_str(app, DigiviceStrGateRetreat));
    const char* train = digivice_app_str(app, DigiviceStrMenuTrain);
    int32_t tx = 120 - digivice_draw_text_width(canvas, train);
    digivice_draw_text(canvas, tx, 59, train);
    digivice_draw_text(canvas, 123, 59, ">");
    if(app->notice_frames) digivice_draw_label(canvas, 20, digivice_app_str(app, app->notice));
}

void digivice_expedition_draw(Canvas* canvas, DigiviceApp* app) {
    switch(app->screen) {
    case DigiviceScreenExpedition:
        draw_trail(canvas, app);
        if(app->notice_frames) digivice_draw_label(canvas, 12, digivice_app_str(app, app->notice));
        break;
    case DigiviceScreenMap:
        draw_map(canvas, app);
        break;
    case DigiviceScreenHike:
        draw_hike(canvas, app);
        break;
    case DigiviceScreenMidway:
        draw_midway(canvas, app);
        break;
    case DigiviceScreenGate:
        draw_gate(canvas, app);
        break;
    default:
        break;
    }
}
