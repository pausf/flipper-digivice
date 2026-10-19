#include "digivice_app_i.h"
#include <digivice/digivice_notify.h>
#include <furi_hal.h>
#include <notification/notification_messages.h>

#include <stdio.h>

// Frame marks of the 100 ms clock.
#define ENCOUNTER_FRAMES 40
#define LOCK_FRAMES      5 // the tier shows after OK, before the aim
#define SHOT_WINDUP      5
#define SHOT_TRAVEL      4
#define SHOT_IMPACT      5
#define SHOT_FRAMES      (SHOT_WINDUP + SHOT_TRAVEL + SHOT_IMPACT)
#define FADE_FRAMES      12
#define CLEAR_START      20 // the boss "area cleared" line follows the victory banner

// Arena layout (combate.html): pet feet at (30, 50), foe feet at (98, 50).
#define PET_X   30
#define FOE_X   98
#define FEET_Y  50
#define PET_CY  39
#define MOUTH_Y 36
#define GUARD_X 70

static const DigiviceStr tier_str[DigiviceChargeCount] = {
    DigiviceStrBtlShotSmall,
    DigiviceStrBtlShotNormal,
    DigiviceStrBtlShotBig,
};

static void battle_exit(DigiviceApp* app) {
    digivice_expedition_enter(app);
}

static const char* foe_name(const DigiviceApp* app) {
    if(app->enc_kind == DigiviceEncounterBoss) return digivice_expedition_boss_name(app->enc_area);
    if(app->enc_kind == DigiviceEncounterVirux) return "VIRUX";
    return digivice_expedition_foe_name((DigiviceFoe)app->battle.foe);
}

// Glyph-safe prefix: the byte length of the first `glyphs` UTF-8 glyphs of `text`.
static size_t utf8_prefix(const char* text, uint32_t glyphs) {
    size_t i = 0;
    while(text[i]) {
        if(((unsigned char)text[i] & 0xC0) != 0x80) {
            if(glyphs == 0) break;
            glyphs--;
        }
        i++;
    }
    return i;
}

static void foe_led(DigiviceApp* app) {
    static const NotificationSequence* const leds[DigiviceAttrCount] = {
        &sequence_set_only_blue_255,
        &sequence_set_only_red_255,
        &sequence_set_only_green_255,
        &sequence_blink_white_100,
    };
    uint8_t attr = app->battle.foe_attr < DigiviceAttrCount ? app->battle.foe_attr :
                                                               DigiviceAttrNulo;
    notification_message(app->notification, leds[attr]);
}

void digivice_encounter_start(DigiviceApp* app, DigiviceEncounterKind kind) {
    uint32_t now = furi_hal_rtc_get_timestamp();
    DigiviceResult result = digivice_rules_can_battle(&app->state, now);
    if(result != DigiviceResultOk) {
        digivice_app_refusal(app, result);
        return;
    }
    app->enc_kind = (uint8_t)kind;
    app->enc_area = app->state.area;
    app->offered_kind = (uint8_t)kind;
    app->offered_expires = kind == DigiviceEncounterTrail ? app->state.enc_expires :
                           kind == DigiviceEncounterVirux ? app->state.virux_expires :
                                                            app->state.cleared;
    digivice_battle_init(&app->battle, &app->state, kind, now);
    app->streak_lost = false;
    app->area_cleared = false;
    digivice_app_set_screen(app, DigiviceScreenEncounter);
    notification_message(app->notification, &digivice_sequence_encounter);
}

static void battle_set_phase(DigiviceApp* app, DigiviceBattlePhase phase) {
    app->phase = phase;
    app->screen_frame = 0;
    if(phase == DigiviceBattlePhaseCharge) {
        app->zone_w = digivice_rules_charge_zone_w(app->state.strength);
        app->zone_start =
            digivice_rules_charge_zone_start(app->zone_w, (uint8_t)furi_hal_random_get());
        app->cursor = 0;
        app->charge_locked = false;
        notification_message(app->notification, &digivice_sequence_charge);
    } else if(phase == DigiviceBattlePhaseAim) {
        app->guard0 = (uint8_t)(furi_hal_random_get() & 1);
        app->aim = DigiviceAimHigh;
        app->aim_locked = false;
    }
}

void digivice_encounter_input(DigiviceApp* app, const InputEvent* event) {
    // Declining before the first charge is always free, even in streak areas.
    if(event->type == InputTypeShort && event->key == InputKeyBack) {
        notification_message(app->notification, &sequence_reset_rgb);
        battle_exit(app);
    }
}

void digivice_encounter_tick(DigiviceApp* app) {
    uint32_t f = app->screen_frame;
    if(f == 16) foe_led(app);
    if(f >= ENCOUNTER_FRAMES) {
        notification_message(app->notification, &sequence_reset_rgb);
        digivice_app_set_screen(app, DigiviceScreenBattle);
        battle_set_phase(app, DigiviceBattlePhaseCharge);
    }
}

// The pet, the foe and their shadows; (ox, oy) is the arena shake.
static void draw_fighters(
    Canvas* canvas,
    DigiviceApp* app,
    int32_t ox,
    int32_t oy,
    int32_t pet_dx,
    int32_t pet_dy,
    int32_t foe_dx,
    int32_t foe_dy,
    DigivicePose pet_pose,
    DigivicePose foe_pose) {
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_ring(canvas, PET_X + ox, FEET_Y + 1 + oy, 14, 2);
    digivice_draw_ring(canvas, FOE_X + ox, FEET_Y + 1 + oy, 16, 2);
    digivice_draw_form(
        canvas,
        (DigiviceForm)app->state.form,
        PET_X + ox + pet_dx,
        PET_CY + oy + pet_dy,
        app->frame,
        pet_pose,
        false);
    int32_t fx = FOE_X + ox + foe_dx;
    int32_t fy = FEET_Y + oy + foe_dy;
    if(app->enc_kind == DigiviceEncounterBoss) {
        digivice_draw_boss(
            canvas, app->enc_area, (DigiviceForm)app->state.form, fx, fy, app->frame, false);
    } else if(app->enc_kind == DigiviceEncounterVirux) {
        digivice_draw_virux(canvas, fx, fy - 14, app->frame);
    } else {
        digivice_draw_foe(
            canvas, (DigiviceFoe)app->battle.foe, fx, fy, app->frame, foe_pose, true, false);
    }
}

static void draw_panel(Canvas* canvas) {
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 51, 128, 13);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, 51, 127, 51);
}

static int8_t player_edge(const DigiviceApp* app) {
    return digivice_rules_attr_adv(app->battle.player_attr, app->battle.foe_attr);
}

void digivice_encounter_draw(Canvas* canvas, DigiviceApp* app) {
    uint32_t f = app->screen_frame;
    bool boss = app->enc_kind == DigiviceEncounterBoss;
    char line[48];

    if(f < 16) {
        int32_t shake = boss && f < 8 ? (int32_t)(f / 2) % 3 - 1 : 0;
        digivice_draw_scene(
            canvas, digivice_app_scene(app), 0, app->frame, digivice_app_day_minute());
        digivice_draw_form(
            canvas,
            (DigiviceForm)app->state.form,
            34 + shake,
            PET_CY,
            app->frame,
            DigivicePoseIdle,
            false);
        if(boss) {
            if(f >= 8) {
                // The silhouette rises from below.
                int32_t rise = (int32_t)(16 - f) * 6;
                digivice_draw_boss(
                    canvas,
                    app->enc_area,
                    (DigiviceForm)app->state.form,
                    96,
                    FEET_Y + rise,
                    app->frame,
                    true);
                if(f == 12) digivice_draw_invert(canvas, 0, 0, 128, 64);
            } else {
                digivice_draw_hazard(canvas, 9, app->frame);
                digivice_draw_hazard(canvas, 56, app->frame);
                if(f % 2 == 0) {
                    digivice_draw_label(canvas, 28, digivice_app_str(app, DigiviceStrEncAlert));
                }
            }
        } else if(f < 8) {
            digivice_draw_exclam_bubble(canvas, 40, 14 - (int32_t)(f % 4 < 2));
        } else {
            // Wipe in 16 strips, with glitch rows and two inverted frames.
            for(int32_t k = 0; k < (int32_t)(f - 8) * 2; k++)
                digivice_draw_box(canvas, 0, k * 4, 128, 4);
            digivice_draw_glitch(canvas, app->frame, 3);
            if(f == 10 || f == 13) digivice_draw_invert(canvas, 0, 0, 128, 64);
        }
        return;
    }

    // From frame 16 on: the arena with the fighters sliding in.
    canvas_set_color(canvas, ColorBlack);
    for(int32_t y = 11; y < 50; y += 8)
        for(int32_t x = 4; x < 128; x += 8)
            canvas_draw_dot(canvas, x, y);
    int32_t slide = f < 22 ? 50 : f < 29 ? (int32_t)(29 - f) * 7 : 0;
    draw_fighters(canvas, app, 0, 0, -slide, 0, slide, 0, DigivicePoseIdle, DigivicePoseIdle);

    if(f < 22) {
        // VS band (boss: the JEFE banner) wiping in diagonally.
        int32_t w = (int32_t)(f - 15) * 22;
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_fill_triangle(canvas, 0, 18, w, 18, w - 12, 40);
        digivice_draw_fill_triangle(canvas, 0, 18, w - 12, 40, 0, 40);
        canvas_set_color(canvas, ColorWhite);
        if(boss) {
            digivice_draw_text(canvas, 6, 22, digivice_app_str(app, DigiviceStrEncBoss));
            digivice_draw_text(canvas, 6, 30, foe_name(app));
            snprintf(
                line,
                sizeof(line),
                "%s%u",
                digivice_app_str(app, DigiviceStrEncZone),
                (unsigned)(app->enc_area + 1));
            digivice_draw_text(canvas, 50, 30, line);
        } else {
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str(canvas, 54, 34, "VS");
        }
        canvas_set_color(canvas, ColorBlack);
    }

    draw_panel(canvas);
    if(f >= 29) {
        uint32_t typed = (f - 29) * 3;
        if(boss) {
            snprintf(line, sizeof(line), "%s", foe_name(app));
        } else {
            snprintf(
                line,
                sizeof(line),
                "%s%s%s",
                digivice_app_str(app, DigiviceStrEncWildPre),
                foe_name(app),
                digivice_app_str(app, DigiviceStrEncWildPost));
        }
        line[utf8_prefix(line, typed)] = '\0';
        digivice_draw_attr_icon(canvas, (DigiviceAttr)app->battle.foe_attr, 3, 53);
        digivice_draw_text(canvas, 13, 53, line);
        int8_t edge = player_edge(app);
        DigiviceStr edge_str = edge > 0 ? DigiviceStrEncEdge :
                               edge < 0 ? DigiviceStrEncDisedge :
                                          DigiviceStrEncNeutral;
        snprintf(
            line,
            sizeof(line),
            "%s%u",
            digivice_app_str(app, DigiviceStrEncPow),
            (unsigned)app->battle.foe_power);
        digivice_draw_text(canvas, 3, 59, line);
        digivice_draw_text(canvas, 40, 59, digivice_app_str(app, edge_str));
    }
    digivice_draw_text(canvas, 92, 59, digivice_app_str(app, DigiviceStrEncRetreatHint));
}

static void battle_finish(DigiviceApp* app, DigiviceBattleResult result) {
    app->result = (uint8_t)result;
    uint8_t cleared_before = app->state.cleared;
    app->reward = digivice_battle_finish(
        app->digivice,
        (DigiviceEncounterKind)app->enc_kind,
        result,
        app->battle.shield_used,
        app->offered_expires);
    digivice_get_state(app->digivice, &app->state);
    app->streak_lost = result == DigiviceBattleResultLose &&
                       app->enc_kind != DigiviceEncounterVirux &&
                       digivice_expedition_streak_area(app->enc_area);
    app->area_cleared = app->enc_kind == DigiviceEncounterBoss &&
                        result == DigiviceBattleResultWin && app->state.cleared != cleared_before;
    if(result == DigiviceBattleResultWin) {
        notification_message(app->notification, &digivice_sequence_win);
    } else if(result == DigiviceBattleResultLose) {
        notification_message(app->notification, &digivice_sequence_lose);
    } else {
        notification_message(app->notification, &sequence_reset_rgb);
    }
    battle_set_phase(app, DigiviceBattlePhaseEnd);
}

static void battle_after_shot(DigiviceApp* app, bool player) {
    DigiviceBattleOutcome outcome = digivice_battle_outcome(&app->battle);
    if(outcome == DigiviceBattleWon) {
        battle_finish(app, DigiviceBattleResultWin);
    } else if(outcome == DigiviceBattleLost) {
        battle_finish(app, DigiviceBattleResultLose);
    } else if(outcome == DigiviceBattleDrawn) {
        battle_finish(app, DigiviceBattleResultDraw);
    } else if(player) {
        app->aim = (uint8_t)(furi_hal_random_get() & 1);
        app->shot = (uint8_t)digivice_battle_foe_shot(&app->battle, digivice_app_rng_pct());
        battle_set_phase(app, DigiviceBattlePhaseFoeShot);
    } else {
        battle_set_phase(app, DigiviceBattlePhaseCharge);
    }
}

static void battle_lock_charge(DigiviceApp* app) {
    app->charge_locked = true;
    app->charge = (uint8_t)digivice_rules_charge_tier(app->cursor, app->zone_start, app->zone_w);
    app->screen_frame = 0;
}

static void battle_fire(DigiviceApp* app, uint8_t aim) {
    app->aim = aim;
    app->aim_locked = true;
    app->aim_frame = app->screen_frame;
    bool blocked = digivice_rules_shot_blocked(aim, app->guard0, app->aim_frame);
    app->shot = (uint8_t)digivice_battle_player_shot(
        &app->battle,
        (DigiviceCharge)app->charge,
        blocked,
        digivice_app_rng_pct(),
        digivice_app_rng_pct());
    notification_message(app->notification, &sequence_reset_rgb);
    battle_set_phase(app, DigiviceBattlePhasePlayerShot);
}

static void battle_try_retreat(DigiviceApp* app) {
    if(digivice_battle_retreat_allowed(&app->state, (DigiviceEncounterKind)app->enc_kind)) {
        battle_finish(app, DigiviceBattleResultFled);
    } else {
        digivice_app_notice(app, DigiviceStrBtlNoEscape);
    }
}

void digivice_battle_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;
    switch(app->phase) {
    case DigiviceBattlePhaseCharge:
        if(event->key == InputKeyOk && !app->charge_locked) {
            battle_lock_charge(app);
        } else if(event->key == InputKeyBack) {
            battle_try_retreat(app);
        }
        break;
    case DigiviceBattlePhaseAim:
        if(event->key == InputKeyUp) {
            battle_fire(app, DigiviceAimHigh);
        } else if(event->key == InputKeyDown) {
            battle_fire(app, DigiviceAimLow);
        } else if(event->key == InputKeyBack) {
            battle_try_retreat(app);
        }
        break;
    case DigiviceBattlePhaseEnd:
        if(event->key == InputKeyOk || event->key == InputKeyBack) battle_exit(app);
        break;
    default:
        break;
    }
}

void digivice_battle_tick(DigiviceApp* app) {
    uint32_t f = app->screen_frame;
    switch(app->phase) {
    case DigiviceBattlePhaseCharge:
        if(!app->charge_locked) {
            app->cursor = digivice_rules_meter_cursor(f);
            if(f >= DIGIVICE_CHARGE_FRAMES) battle_lock_charge(app);
        } else if(f >= LOCK_FRAMES) {
            battle_set_phase(app, DigiviceBattlePhaseAim);
        }
        break;
    case DigiviceBattlePhaseAim:
        if(f >= DIGIVICE_AIM_FRAMES) battle_fire(app, DigiviceAimHigh);
        break;
    case DigiviceBattlePhasePlayerShot:
    case DigiviceBattlePhaseFoeShot: {
        bool player = app->phase == DigiviceBattlePhasePlayerShot;
        bool landed = app->shot == DigiviceShotHit || app->shot == DigiviceShotGuardBroken;
        if(f == SHOT_WINDUP + SHOT_TRAVEL && landed) {
            notification_message(app->notification, &digivice_sequence_hit);
        }
        if(f >= SHOT_FRAMES) battle_after_shot(app, player);
        break;
    }
    case DigiviceBattlePhaseEnd:
        if(app->area_cleared && f >= CLEAR_START + FADE_FRAMES) battle_exit(app);
        break;
    }
}

static void draw_hud(Canvas* canvas, DigiviceApp* app) {
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, 128, 8);
    canvas_set_color(canvas, ColorWhite);
    const char* pet = digivice_form_name((DigiviceForm)app->state.form);
    const char* foe = foe_name(app);
    int32_t pet_w = digivice_draw_text_width(canvas, pet);
    int32_t foe_x = 126 - digivice_draw_text_width(canvas, foe);
    digivice_draw_text(canvas, 2, 1, pet);
    digivice_draw_text(canvas, foe_x, 1, foe);
    digivice_draw_text(canvas, 58, 1, "VS");
    // Three hit boxes per side, right after each name so 9-glyph names still fit.
    for(int32_t i = 0; i < DIGIVICE_HITS_TO_WIN; i++) {
        int32_t px = 2 + pet_w + 3 + i * 6;
        int32_t fx = foe_x - 8 - i * 6;
        if(i < app->battle.player_hits) {
            canvas_draw_box(canvas, px, 2, 5, 5);
        } else {
            canvas_draw_frame(canvas, px, 2, 5, 5);
        }
        if(i < app->battle.foe_hits) {
            canvas_draw_box(canvas, fx, 2, 5, 5);
        } else {
            canvas_draw_frame(canvas, fx, 2, 5, 5);
        }
    }
    canvas_set_color(canvas, ColorBlack);
}

// Projectile path for the shot on screen: frames SHOT_WINDUP .. SHOT_WINDUP + SHOT_TRAVEL.
static void draw_shot(Canvas* canvas, DigiviceApp* app, bool player, uint32_t f) {
    int32_t from = player ? PET_X + 10 : FOE_X - 12;
    int32_t to = player ? (app->shot == DigiviceShotBlocked ? GUARD_X : FOE_X - 8) : PET_X + 6;
    int32_t ty = MOUTH_Y + (app->aim == DigiviceAimHigh ? -6 : 6);
    uint8_t size = player ? app->battle.player_shot : app->battle.foe_shot;
    if(player && app->charge == DigiviceChargeBig && size < DIGIVICE_SHOT_BEAM) size += 2;
    uint8_t attr = player ? app->battle.player_attr : app->battle.foe_attr;
    int8_t dir = player ? 1 : -1;
    bool missed = app->shot == DigiviceShotDodge;
    if(f < SHOT_WINDUP) {
        digivice_draw_dash_ring(canvas, from, MOUTH_Y, 4 + (int32_t)f, app->frame);
        return;
    }
    int32_t t = (int32_t)(f - SHOT_WINDUP);
    if(t < SHOT_TRAVEL) {
        int32_t x = from + (to - from) * t / SHOT_TRAVEL;
        int32_t y = MOUTH_Y + (ty - MOUTH_Y) * t / SHOT_TRAVEL;
        digivice_draw_shot(canvas, (DigiviceAttr)attr, x, y, size, dir, app->frame);
    } else if(missed) {
        // The shot flies on past the target with speed lines.
        int32_t x = to + dir * (t - SHOT_TRAVEL + 1) * 14;
        digivice_draw_shot(canvas, (DigiviceAttr)attr, x, ty, size, dir, app->frame);
        for(int32_t k = 1; k <= 3; k++) {
            int32_t ly = ty - 4 + k * 2;
            digivice_draw_line(canvas, x - dir * (6 + k * 4), ly, x - dir * (2 + k * 4), ly);
        }
    }
}

static void draw_impact(Canvas* canvas, DigiviceApp* app, bool player, uint32_t f) {
    int32_t t = (int32_t)f - (SHOT_WINDUP + SHOT_TRAVEL);
    if(t < 0) return;
    int32_t tx = player ? FOE_X : PET_X;
    int32_t ty = MOUTH_Y + (app->aim == DigiviceAimHigh ? -6 : 6);
    DigiviceShot shot = (DigiviceShot)app->shot;
    if(shot == DigiviceShotShielded) {
        digivice_draw_shield(canvas, PET_X + 12, 26);
        digivice_draw_burst(canvas, PET_X + 14, 30, 4 + t * 2);
        return;
    }
    if(shot == DigiviceShotBlocked || shot == DigiviceShotGuardBroken) {
        digivice_draw_burst(canvas, GUARD_X + 2, ty, 3 + t * 2);
        if(shot == DigiviceShotBlocked) return;
    }
    if(shot == DigiviceShotHit || shot == DigiviceShotGuardBroken) {
        if(t < 2) digivice_draw_invert(canvas, tx - 16, 14, 32, 37);
        digivice_draw_star5(canvas, tx, ty);
        digivice_draw_burst(canvas, tx, ty, 4 + t * 2);
        // "-1" floating up in a small box.
        int32_t ny = 14 - t * 2;
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, tx - 5, ny - 1, 11, 8);
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_text(canvas, tx - 4, ny, "-1");
    }
}

static void draw_shot_lines(Canvas* canvas, DigiviceApp* app, bool player, uint32_t f) {
    char line[48];
    DigiviceShot shot = (DigiviceShot)app->shot;
    bool impact = f >= SHOT_WINDUP + SHOT_TRAVEL;
    if(!player) {
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrBtlFoeTurn));
        if(!impact) return;
        DigiviceStr s = shot == DigiviceShotHit      ? DigiviceStrBtlFoeHit :
                        shot == DigiviceShotShielded ? DigiviceStrBtlShieldHolds :
                                                       DigiviceStrBtlFoeMiss;
        digivice_draw_text(canvas, 3, 59, digivice_app_str(app, s));
        return;
    }
    if(!impact) {
        DigiviceStr s = app->aim == DigiviceAimHigh ? DigiviceStrBtlShotHigh :
                                                      DigiviceStrBtlShotLow;
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, s));
        digivice_draw_text(canvas, 3, 59, digivice_app_str(app, tier_str[app->charge]));
        return;
    }
    if(shot == DigiviceShotHit) {
        // Label + " n/3": the hit count is drawn by the app, not by the string table.
        snprintf(
            line,
            sizeof(line),
            "%s %u/%u",
            digivice_app_str(app, DigiviceStrBtlHit),
            (unsigned)app->battle.player_hits,
            (unsigned)DIGIVICE_HITS_TO_WIN);
        digivice_draw_text(canvas, 3, 53, line);
    } else if(shot == DigiviceShotGuardBroken) {
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrBtlBroken));
    } else if(shot == DigiviceShotBlocked) {
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrBtlBlocked));
    } else {
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrBtlDodge));
        digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrBtlWhiff));
    }
}

static void draw_end(Canvas* canvas, DigiviceApp* app) {
    uint32_t f = app->screen_frame;
    char line[48];
    DigiviceBattleResult result = (DigiviceBattleResult)app->result;
    bool won = result == DigiviceBattleResultWin;
    bool lost = result == DigiviceBattleResultLose;
    int32_t sink = lost ? 2 : 0;

    canvas_set_color(canvas, ColorBlack);
    for(int32_t y = 11; y < 50; y += 8)
        for(int32_t x = 4; x < 128; x += 8)
            canvas_draw_dot(canvas, x, y);
    draw_fighters(
        canvas,
        app,
        0,
        0,
        0,
        sink,
        0,
        0,
        won ? DigivicePoseAttack : DigivicePoseIdle,
        lost ? DigivicePoseAttack : DigivicePoseHurt);
    if(won) {
        // The foe fades out under a white dither.
        uint8_t level = f < FADE_FRAMES ? (uint8_t)(f * 16 / FADE_FRAMES) : 16;
        canvas_set_color(canvas, ColorWhite);
        digivice_draw_dither(canvas, FOE_X - 28, 8, 56, 43, level);
        canvas_set_color(canvas, ColorBlack);
        digivice_draw_sparkle_ring(canvas, PET_X, PET_CY, 16, 10, app->frame);
        // The banner drops in over 5 frames; labels stay on screen, never at negative rows.
        int32_t by = f < 5 ? (int32_t)f * 2 : 10;
        digivice_draw_label(canvas, by, digivice_app_str(app, DigiviceStrBtlVictory));
        if(app->reward.item == DigiviceItemShield && f >= 8) {
            int32_t bounce = f < 14 ? (int32_t)((14 - f) % 4) : 0;
            digivice_draw_shield(canvas, 62, 22 - bounce);
        }
    } else if(lost) {
        int32_t lx = f < 5 ? 5 + (int32_t)f * 8 : 45;
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, lx - 3, 10, 40, 8);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, lx - 3, 10, 40, 8);
        digivice_draw_text(canvas, lx, 11, digivice_app_str(app, DigiviceStrBtlDefeat));
        if(f >= 6) {
            digivice_draw_bandage(canvas, PET_X + 4, PET_CY - 12);
            int32_t r = 3 + (int32_t)(f - 6) * 2;
            if(f < 9) digivice_draw_burst(canvas, PET_X + 7, PET_CY - 9, r);
        }
    }

    draw_panel(canvas);
    if(won) {
        uint32_t shown = f < 10 ? app->reward.exp * f / 10 : app->reward.exp;
        snprintf(
            line,
            sizeof(line),
            "+%lu %s",
            (unsigned long)shown,
            digivice_app_str(app, DigiviceStrExp));
        digivice_draw_text(canvas, 3, 53, line);
        if(app->area_cleared && f >= CLEAR_START) {
            digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrBtlAreaClear));
        } else if(app->reward.item == DigiviceItemShield && f >= 8) {
            digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrBtlItemShield));
        }
    } else if(lost) {
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrBtlHurt));
        if(app->streak_lost) {
            digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrBtlStreakLost));
        }
    } else {
        DigiviceStr s = result == DigiviceBattleResultDraw ? DigiviceStrBtlDraw :
                                                             DigiviceStrBtlRetreated;
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, s));
    }
}

void digivice_battle_draw(Canvas* canvas, DigiviceApp* app) {
    if(app->phase == DigiviceBattlePhaseEnd) {
        draw_end(canvas, app);
        draw_hud(canvas, app);
        return;
    }
    uint32_t f = app->screen_frame;
    bool player = app->phase == DigiviceBattlePhasePlayerShot;
    bool shooting = player || app->phase == DigiviceBattlePhaseFoeShot;
    int32_t impact = shooting ? (int32_t)f - (SHOT_WINDUP + SHOT_TRAVEL) : -1;
    bool landed = app->shot == DigiviceShotHit || app->shot == DigiviceShotGuardBroken;

    // The arena shakes 2 px on the first impact frame and 1 px on the next three.
    int32_t ox = 0;
    int32_t oy = 0;
    if(landed && impact >= 0 && impact < 4) {
        int32_t amp = impact == 0 ? 2 : 1;
        ox = (app->frame % 2) ? amp : -amp;
        oy = (app->frame % 3) ? 0 : amp;
    }
    // A dodging target hops in a sine arc of up to 10 px.
    static const int8_t hop[SHOT_IMPACT] = {-4, -8, -10, -8, -4};
    int32_t pet_dy = 0;
    int32_t foe_dy = 0;
    if(app->shot == DigiviceShotDodge && impact >= 0 && impact < SHOT_IMPACT) {
        if(player) {
            foe_dy = hop[impact];
        } else {
            pet_dy = hop[impact];
        }
    }

    canvas_set_color(canvas, ColorBlack);
    for(int32_t y = 11; y < 50; y += 8)
        for(int32_t x = 4; x < 128; x += 8)
            canvas_draw_dot(canvas, x + ox, y + oy);
    DigivicePose pet_pose = DigivicePoseIdle;
    DigivicePose foe_pose = DigivicePoseIdle;
    if(player && f < SHOT_WINDUP + SHOT_TRAVEL) pet_pose = DigivicePoseAttack;
    if(!player && shooting && f < SHOT_WINDUP + SHOT_TRAVEL) foe_pose = DigivicePoseAttack;
    if(player && landed && impact >= 0) foe_pose = DigivicePoseHurt;
    bool charged = app->phase == DigiviceBattlePhaseCharge && app->charge_locked;
    if(charged) pet_pose = DigivicePoseAttack;
    draw_fighters(canvas, app, ox, oy, 0, pet_dy, 0, foe_dy, pet_pose, foe_pose);

    if(charged) {
        // The dash ring around the mouth grows with the tier.
        int32_t r = 4 + app->charge * 3 + (int32_t)f;
        digivice_draw_dash_ring(canvas, PET_X + 10, MOUTH_Y, r, app->frame);
    }
    // The foe guard flickers during the aim and freezes where the shot lands.
    if(app->phase == DigiviceBattlePhaseAim || player) {
        uint32_t gf = app->phase == DigiviceBattlePhaseAim ?
                          f :
                          app->aim_frame + DIGIVICE_IMPACT_LAG_FRAMES;
        uint8_t guard = digivice_rules_guard_at(app->guard0, gf);
        bool burst = player && app->shot == DigiviceShotGuardBroken && impact >= 0;
        int32_t gy = (guard == DigiviceAimHigh ? 18 : 34) + oy;
        if(!burst) digivice_draw_shield(canvas, GUARD_X + ox, gy);
    }
    if(shooting) {
        draw_shot(canvas, app, player, f);
        draw_impact(canvas, app, player, f);
    }

    draw_panel(canvas);
    if(app->phase == DigiviceBattlePhaseCharge) {
        // FUERZA and one heart per zone segment above the minimum.
        digivice_draw_text(canvas, 76, 12, digivice_app_str(app, DigiviceStrProfileStrength));
        for(int32_t i = 0; i <= app->zone_w - DIGIVICE_ZONE_BASE_W; i++)
            digivice_draw_heart(canvas, 101 + i * 6, 12);
        if(app->charge_locked) {
            digivice_draw_text(canvas, 3, 53, digivice_app_str(app, tier_str[app->charge]));
        } else {
            digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrBtlCharge));
        }
        digivice_draw_charge_meter(canvas, 3, 59, app->zone_start, app->zone_w, app->cursor);
        digivice_draw_ok_button(canvas, 109, 53, app->charge_locked);
    } else if(app->phase == DigiviceBattlePhaseAim) {
        bool blink = (app->frame % 4) < 2;
        digivice_draw_arrow(canvas, 3, 14, true, blink);
        digivice_draw_arrow(canvas, 3, 40, false, blink);
        digivice_draw_text(canvas, 3, 53, digivice_app_str(app, DigiviceStrBtlAim));
        if(digivice_battle_retreat_allowed(&app->state, (DigiviceEncounterKind)app->enc_kind)) {
            digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrEncRetreatHint));
        } else {
            digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrBtlRule));
        }
    } else {
        if(player && f < SHOT_WINDUP) {
            // The pressed arrow stays filled through the wind-up.
            bool high = app->aim == DigiviceAimHigh;
            digivice_draw_arrow(canvas, 3, high ? 14 : 40, high, true);
        }
        draw_shot_lines(canvas, app, player, f);
    }
    if(app->notice_frames) digivice_draw_label(canvas, 20, digivice_app_str(app, app->notice));
    draw_hud(canvas, app);
}
