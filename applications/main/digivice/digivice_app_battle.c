#include "digivice_app_i.h"
#include <digivice/digivice_notify.h>
#include <notification/notification_messages.h>

#include <stdio.h>

static const DigiviceStr choice_labels[DigiviceBattleChoiceCount] = {
    DigiviceStrBattleAttack,
    DigiviceStrBattleDefend,
    DigiviceStrBattleFlee,
};

static void digivice_battle_set_phase(DigiviceApp* app, DigiviceBattlePhase phase) {
    app->phase = phase;
    app->screen_frame = 0;

    switch(phase) {
    case DigiviceBattlePhaseAim:
        digivice_meter_start(&app->meter);
        notification_message(app->notification, &digivice_sequence_charge);
        app->message = digivice_app_str(app, DigiviceStrBattleAim);
        break;
    case DigiviceBattlePhaseFoeHit:
        app->last_damage = digivice_battle_foe_attack(&app->battle);
        notification_message(app->notification, &digivice_sequence_hit);
        app->message = digivice_app_str(app, DigiviceStrBattleFoeStrikes);
        break;
    case DigiviceBattlePhaseChoose:
        app->message = NULL;
        break;
    default:
        break;
    }
}

static void
    digivice_battle_end(DigiviceApp* app, DigiviceBattleResult result, DigiviceStr message) {
    app->result = result;
    app->message = digivice_app_str(app, message);
    if(result == DigiviceBattleResultWin) {
        notification_message(app->notification, &digivice_sequence_win);
    }
    digivice_battle_set_phase(app, DigiviceBattlePhaseEnd);
}

static void digivice_battle_fire(DigiviceApp* app, bool in_zone) {
    app->last_damage =
        digivice_battle_player_attack(&app->battle, &app->state, in_zone, digivice_app_rng_pct());
    if(app->last_damage) {
        notification_message(app->notification, &digivice_sequence_hit);
        app->message =
            digivice_app_str(app, in_zone ? DigiviceStrBattleCrit : DigiviceStrBattleGlancing);
    } else {
        notification_message(app->notification, &sequence_reset_rgb);
        app->message = digivice_app_str(app, DigiviceStrBattleMiss);
    }
    digivice_battle_set_phase(app, DigiviceBattlePhasePlayerHit);
}

void digivice_battle_enter(DigiviceApp* app) {
    digivice_battle_init(&app->battle, &app->state);
    app->choice = DigiviceBattleChoiceAttack;
    app->message = digivice_app_str(app, DigiviceStrBattleAppeared);
    digivice_app_set_screen(app, DigiviceScreenBattle);
    digivice_battle_set_phase(app, DigiviceBattlePhaseIntro);
}

void digivice_battle_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;

    switch(app->phase) {
    case DigiviceBattlePhaseIntro:
        if(event->key == InputKeyOk) digivice_battle_set_phase(app, DigiviceBattlePhaseChoose);
        break;
    case DigiviceBattlePhaseChoose:
        if(event->key == InputKeyLeft) {
            app->choice =
                (app->choice + DigiviceBattleChoiceCount - 1) % DigiviceBattleChoiceCount;
        } else if(event->key == InputKeyRight) {
            app->choice = (app->choice + 1) % DigiviceBattleChoiceCount;
        } else if(event->key == InputKeyOk) {
            if(app->choice == DigiviceBattleChoiceAttack) {
                digivice_battle_set_phase(app, DigiviceBattlePhaseAim);
            } else if(app->choice == DigiviceBattleChoiceDefend) {
                digivice_battle_defend(&app->battle);
                digivice_battle_set_phase(app, DigiviceBattlePhaseFoeHit);
            } else if(digivice_battle_flee(digivice_app_rng_pct())) {
                digivice_battle_end(app, DigiviceBattleResultFled, DigiviceStrBattleFled);
            } else {
                digivice_battle_set_phase(app, DigiviceBattlePhaseFoeHit);
            }
        }
        break;
    case DigiviceBattlePhaseAim:
        if(event->key == InputKeyOk) digivice_battle_fire(app, digivice_meter_hit(&app->meter));
        break;
    case DigiviceBattlePhaseEnd:
        if(event->key == InputKeyOk || event->key == InputKeyBack) {
            digivice_battle_finish(app->digivice, app->result);
            digivice_app_set_screen(app, DigiviceScreenMain);
            digivice_app_refresh(app);
        }
        break;
    default:
        break;
    }
}

void digivice_battle_tick(DigiviceApp* app) {
    switch(app->phase) {
    case DigiviceBattlePhaseIntro:
        if(app->screen_frame >= 15) digivice_battle_set_phase(app, DigiviceBattlePhaseChoose);
        break;
    case DigiviceBattlePhaseAim:
        digivice_meter_step(&app->meter);
        if(app->screen_frame >= DIGIVICE_APP_AIM_FRAMES) digivice_battle_fire(app, false);
        break;
    case DigiviceBattlePhasePlayerHit:
        if(app->screen_frame < DIGIVICE_APP_HIT_FRAMES) break;
        if(digivice_battle_outcome(&app->battle) == DigiviceBattleWon) {
            digivice_battle_end(app, DigiviceBattleResultWin, DigiviceStrBattleWon);
        } else {
            digivice_battle_set_phase(app, DigiviceBattlePhaseFoeHit);
        }
        break;
    case DigiviceBattlePhaseFoeHit:
        if(app->screen_frame < DIGIVICE_APP_HIT_FRAMES) break;
        if(digivice_battle_outcome(&app->battle) == DigiviceBattleLost) {
            digivice_battle_end(app, DigiviceBattleResultLose, DigiviceStrBattleLost);
        } else {
            digivice_battle_set_phase(app, DigiviceBattlePhaseChoose);
        }
        break;
    default:
        break;
    }
}

static void digivice_battle_draw_textbox(Canvas* canvas, DigiviceApp* app) {
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 50, 128, 14);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, 50, 127, 50);
    canvas_draw_line(canvas, 0, 50, 0, 53);
    canvas_draw_line(canvas, 127, 50, 127, 53);

    if(app->phase == DigiviceBattlePhaseChoose) {
        for(uint8_t i = 0; i < DigiviceBattleChoiceCount; i++) {
            int32_t x = 6 + i * 42;
            digivice_draw_text(canvas, x, 55, digivice_app_str(app, choice_labels[i]));
            // 31 px of text (DEFENDER) plus a 2 px gap on each side.
            if(i == app->choice) digivice_draw_brackets(canvas, x - 3, 53, x + 33, 62, 2);
        }
    } else if(app->phase == DigiviceBattlePhaseAim) {
        digivice_draw_meter(canvas, 14, 56, app->meter.zone_start, app->meter.cursor);
    } else if(app->message) {
        digivice_draw_text(canvas, 2, 52, ">");
        digivice_draw_text(canvas, 7, 52, app->message);
        if(app->phase == DigiviceBattlePhaseEnd && (app->frame % 6) < 3) {
            digivice_draw_fill_triangle(canvas, 118, 58, 124, 58, 121, 61);
        }
    }
}

void digivice_battle_draw(Canvas* canvas, DigiviceApp* app) {
    canvas_set_color(canvas, ColorBlack);
    for(int32_t y = 4; y < 50; y += 8) {
        for(int32_t x = 4; x < 128; x += 8)
            canvas_draw_dot(canvas, x, y);
    }

    uint32_t f = app->screen_frame;
    bool player_hit = app->phase == DigiviceBattlePhasePlayerHit && app->last_damage;
    bool foe_hit = app->phase == DigiviceBattlePhaseFoeHit;

    digivice_draw_ring(canvas, 96, 29, 17, 4);
    digivice_draw_ring(canvas, 96, 29, 10, 2);
    digivice_draw_ring(canvas, 32, 48, 18, 4);

    // The VIRUX lunges at the pet when it strikes.
    int32_t ex = 96;
    int32_t ey = 19;
    if(foe_hit && f < 6) {
        static const int8_t lunge[6] = {0, 12, 24, 30, 24, 12};
        ex -= lunge[f];
        ey += lunge[f] / 2;
    }
    if(!(player_hit && f > 1 && f < 8 && (f % 2))) digivice_draw_virux(canvas, ex, ey, app->frame);
    int32_t r = 13 + (int32_t)(app->frame % 4) / 2;
    digivice_draw_brackets(canvas, ex - r - 4, ey - r, ex + r + 4, ey + r, 3);

    int32_t px = 32;
    if(app->phase == DigiviceBattlePhasePlayerHit && f < 4) px += 4;
    DigivicePose pose =
        (app->phase == DigiviceBattlePhaseAim || app->phase == DigiviceBattlePhasePlayerHit) ?
            DigivicePoseAttack :
            DigivicePoseIdle;
    if(!(foe_hit && f > 5 && (f % 2))) {
        digivice_draw_form(canvas, (DigiviceForm)app->state.form, px, 40, app->frame, pose, false);
    }

    if(player_hit && f < 5) {
        // Beam from the pet's mouth to the VIRUX.
        int32_t bx = 44 + (int32_t)f * (ex - 56) / 5;
        int32_t by = 36 + (int32_t)f * (ey - 36) / 5;
        canvas_draw_line(canvas, 44, 36, bx, by);
        canvas_draw_line(canvas, 44, 37, bx, by + 1);
        canvas_draw_circle(canvas, bx, by, 3);
    }

    char number[8];
    if((player_hit || foe_hit) && f < DIGIVICE_APP_HIT_FRAMES) {
        snprintf(number, sizeof(number), "-%u", (unsigned)app->last_damage);
        int32_t nx = player_hit ? ex - 6 : 26;
        int32_t ny = (player_hit ? ey - 21 : 20) - (int32_t)f / 2;
        if(ny < 1) ny = 1; // the box must stay on screen
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, nx - 2, ny - 1, 18, 8);
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_text(canvas, nx, ny, number);
    }

    digivice_draw_hp_box(canvas, 2, 2, "VIRUX", app->battle.foe_hp, app->battle.foe_hp_max);
    digivice_draw_hp_box(
        canvas,
        68,
        33,
        digivice_form_name((DigiviceForm)app->state.form),
        app->battle.player_hp,
        app->battle.player_hp_max);

    digivice_battle_draw_textbox(canvas, app);

    if((player_hit || foe_hit) && f < 4) digivice_draw_glitch(canvas, app->frame, 4);
}
