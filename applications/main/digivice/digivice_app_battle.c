#include "digivice_app_i.h"
#include <digivice/digivice_notify.h>
#include <furi_hal.h>
#include <notification/notification_messages.h>

#include <stdio.h>
#include <string.h>

// Frame marks of the 100 ms clock.
#define ENCOUNTER_FRAMES 40
#define LOCK_FRAMES      5 // the timing bar shows its result after OK, before the turn
#define END_INPUT_FRAMES 8 // mashing OK during the finish must not skip the result screen
#define FADE_FRAMES      12
#define CLEAR_START      20 // the boss "area cleared" line follows the victory banner
#define CAPTURE_DIZZY_END 12
#define CAPTURE_RING_END  40

// One step of a turn: the use line, the projectile or effect, the impact, then the messages.
#define STEP_RING        7
#define STEP_SHOT        12
#define STEP_HIT         16
#define STEP_MSGS        21
#define MSG_FRAMES       10
#define STEP_ITEM_FRAMES 12

// Arena layout of the duel; the lead-in and the end screen keep the full-screen one below.
#define ARENA_PET_X     22
#define ARENA_PET_FEET  41
#define ARENA_PET_CY    30
#define ARENA_FOE_X     100
#define ARENA_FOE_FEET  26
#define ARENA_BOSS_FEET 29 // bosses are up to 48 px tall: the top of the sprite is clipped less

// Arena layout of the lead-in and the end screen: pet feet at (30, 50), foe feet at (98, 50).
#define PET_X   30
#define FOE_X   98
#define FEET_Y  50
#define PET_CY  39

static bool capture_active(const DigiviceApp* app) {
    return app->result == DigiviceBattleResultWin &&
           (app->reward.capture == DigiviceCaptureCaught ||
            app->reward.capture == DigiviceCaptureEscaped);
}

static void battle_exit(DigiviceApp* app) {
    digivice_expedition_enter(app);
}

static const char* foe_name(const DigiviceApp* app) {
    if(app->enc_kind == DigiviceEncounterBoss) return digivice_expedition_boss_name(app->enc_area);
    if(app->enc_kind == DigiviceEncounterVirux) return "VIRUX";
    if(app->duel.foe_class == DigiviceFoeClassHomage) {
        return digivice_homage_name(app->duel.foe);
    }
    if(app->duel.foe_class == DigiviceFoeClassCity) {
        return digivice_city_monster_name(app->duel.foe);
    }
    return digivice_expedition_foe_name((DigiviceFoe)app->duel.foe);
}

// Partners keep the gold special LEDs; city monsters use the attribute LED like any foe.
static bool is_homage(const DigiviceApp* app) {
    return app->enc_kind == DigiviceEncounterTrail &&
           app->duel.foe_class == DigiviceFoeClassHomage;
}

static bool is_special(const DigiviceApp* app) {
    return app->enc_kind == DigiviceEncounterTrail &&
           app->duel.foe_class != DigiviceFoeClassPool;
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

// The lead-in banner of a special encounter; `text` holds the longest city line.
static const char* special_banner(const DigiviceApp* app, char* text, size_t size) {
    if(app->duel.foe_class == DigiviceFoeClassCity) {
        snprintf(
            text,
            size,
            "%s%s",
            digivice_app_str(app, DigiviceStrCityEvent),
            digivice_city_name((uint8_t)(app->duel.foe + 1)));
        return text;
    }
    return digivice_app_str(app, DigiviceStrEncSpecial);
}

static void foe_led(DigiviceApp* app) {
    static const NotificationSequence* const leds[DigiviceAttrCount] = {
        &sequence_set_only_blue_255,
        &sequence_set_only_red_255,
        &sequence_set_only_green_255,
        &sequence_blink_white_100,
    };
    uint8_t attr = app->duel.f[DigiviceSideFoe].attr < DigiviceAttrCount ?
                        app->duel.f[DigiviceSideFoe].attr :
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
    digivice_duel_init(&app->duel, &app->state, kind, now);
    app->menu_sel = 0;
    app->move_sel = 0;
    app->streak_lost = false;
    app->area_cleared = false;
    digivice_app_set_screen(app, DigiviceScreenEncounter);
    notification_message(
        app->notification,
        is_homage(app) ? &digivice_sequence_special : &digivice_sequence_encounter);
}

static void battle_set_phase(DigiviceApp* app, DigiviceBattlePhase phase) {
    app->phase = phase;
    app->screen_frame = 0;
    if(phase == DigiviceBattlePhaseMenu) {
        app->crit_bonus = false;
        for(uint8_t s = 0; s < DigiviceSideCount; s++) app->hp_shown[s] = app->duel.f[s].hp;
    } else if(phase == DigiviceBattlePhaseMoves) {
        app->crit_bonus = false;
    } else if(phase == DigiviceBattlePhaseTiming) {
        app->zone_w = digivice_rules_charge_zone_w(app->state.strength);
        app->zone_start =
            digivice_rules_charge_zone_start(app->zone_w, (uint8_t)furi_hal_random_get());
        app->cursor = 0;
        app->charge_locked = false;
        notification_message(app->notification, &digivice_sequence_charge);
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
    if(f == 16) {
        if(is_homage(app)) {
            notification_message(app->notification, &digivice_sequence_special_hold);
        } else {
            foe_led(app);
        }
    }
    if(f >= ENCOUNTER_FRAMES) {
        notification_message(app->notification, &sequence_reset_rgb);
        digivice_app_set_screen(app, DigiviceScreenBattle);
        battle_set_phase(app, DigiviceBattlePhaseMenu);
    }
}

static void draw_foe_sprite(
    Canvas* canvas,
    DigiviceApp* app,
    int32_t fx,
    int32_t fy,
    DigivicePose pose) {
    if(app->enc_kind == DigiviceEncounterBoss) {
        digivice_draw_boss(
            canvas, app->enc_area, (DigiviceForm)app->state.form, fx, fy, app->frame, false);
    } else if(app->enc_kind == DigiviceEncounterVirux) {
        digivice_draw_virux(canvas, fx, fy - 14, app->frame);
    } else if(app->duel.foe_class == DigiviceFoeClassHomage) {
        digivice_draw_homage(
            canvas, app->duel.foe, fx, fy, app->frame, pose, true, false, false);
    } else if(app->duel.foe_class == DigiviceFoeClassCity) {
        digivice_draw_city_monster(canvas, app->duel.foe, fx, fy, app->frame, pose, true, false);
    } else {
        digivice_draw_foe(
            canvas, (DigiviceFoe)app->duel.foe, fx, fy, app->frame, pose, true, false);
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
    bool end = app->phase == DigiviceBattlePhaseEnd && capture_active(app);
    uint32_t cf = app->screen_frame;
    if(end && app->reward.capture == DigiviceCaptureEscaped && cf > CAPTURE_RING_END) {
        foe_dx += (int32_t)(cf - CAPTURE_RING_END) * 2;
    }
    bool foe_hidden = end && app->reward.capture == DigiviceCaptureCaught && cf >= CAPTURE_RING_END;
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
    if(!foe_hidden) draw_foe_sprite(canvas, app, FOE_X + ox + foe_dx, FEET_Y + oy + foe_dy, foe_pose);
}

static void draw_panel(Canvas* canvas) {
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 51, 128, 13);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, 51, 127, 51);
}

static int8_t player_edge(const DigiviceApp* app) {
    return digivice_rules_attr_adv(
        app->duel.f[DigiviceSidePlayer].attr, app->duel.f[DigiviceSideFoe].attr);
}

void digivice_encounter_draw(Canvas* canvas, DigiviceApp* app) {
    uint32_t f = app->screen_frame;
    bool boss = app->enc_kind == DigiviceEncounterBoss;
    char line[48];
    char banner[32];

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
        digivice_app_draw_companion(canvas, app, 12, 51, false);
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
            if(is_special(app)) {
                digivice_draw_sparkle_ring(canvas, 34, PET_CY, 18, 12, app->frame);
                if(f % 2 == 0) {
                    digivice_draw_label(canvas, 28, special_banner(app, banner, sizeof(banner)));
                }
            } else {
                digivice_draw_exclam_bubble(canvas, 40, 14 - (int32_t)(f % 4 < 2));
            }
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
        if(boss || is_special(app)) {
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
        digivice_draw_attr_icon(canvas, (DigiviceAttr)app->duel.f[DigiviceSideFoe].attr, 3, 53);
        digivice_draw_text(canvas, 13, 53, line);
        int8_t edge = player_edge(app);
        DigiviceStr edge_str = edge > 0 ? DigiviceStrEncEdge :
                               edge < 0 ? DigiviceStrEncDisedge :
                                          DigiviceStrEncNeutral;
        snprintf(line, sizeof(line), "LV%u", (unsigned)app->duel.f[DigiviceSideFoe].level);
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
        app->duel.shield_used,
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

static bool is_item_step(const DigiviceApp* app) {
    return app->turn_item && app->step.actor == DigiviceSidePlayer;
}

static bool shield_available(const DigiviceApp* app) {
    return digivice_duel_shield_ready(&app->duel) && !app->duel.shield_used;
}

static void msg_add(DigiviceApp* app, DigiviceStr id) {
    if(app->msg_count < COUNT_OF(app->msgs)) app->msgs[app->msg_count++] = id;
}

static DigiviceStr fx_msg(uint8_t fx) {
    switch(fx) {
    case DigiviceFxAtkDown:
        return DigiviceStrBtlAtkDown;
    case DigiviceFxDefDown:
        return DigiviceStrBtlDefDown;
    case DigiviceFxHeal:
        return DigiviceStrBtlHealed;
    case DigiviceFxCharge:
        return DigiviceStrBtlCharging;
    case DigiviceFxShield:
        return DigiviceStrBtlGuarding;
    case DigiviceFxGlitch:
        return DigiviceStrBtlGlitched;
    default:
        return DigiviceStrBtlNoEffect;
    }
}

static void step_messages(DigiviceApp* app) {
    const DigiviceStep* step = &app->step;
    app->msg_count = 0;
    if(step->result == DigiviceStepSkipped) {
        msg_add(app, DigiviceStrBtlParalysed);
    } else if(step->result == DigiviceStepMissed) {
        msg_add(app, DigiviceStrBtlFoeMiss);
    } else if(step->result == DigiviceStepShielded) {
        msg_add(app, DigiviceStrBtlShieldHolds);
    } else if(digivice_move(step->move)->power) {
        if(step->crit) msg_add(app, DigiviceStrBtlCrit);
        if(step->edge > 0) {
            msg_add(app, DigiviceStrBtlSuper);
        } else if(step->edge < 0) {
            msg_add(app, DigiviceStrBtlWeak);
        }
        if(step->fx != DigiviceFxNone) msg_add(app, fx_msg(step->fx));
    } else {
        msg_add(app, fx_msg(step->fx));
    }
}

// Resolves one side's move only when its turn on screen starts, so the duel always matches it.
static void step_start(DigiviceApp* app, uint8_t side) {
    app->screen_frame = 0;
    app->msg_count = 0;
    if(app->turn_item && side == DigiviceSidePlayer) {
        digivice_duel_arm_shield(&app->duel);
        app->step = (DigiviceStep){.actor = side, .move = DIGIVICE_MOVE_NONE};
        return;
    }
    DigiviceRolls rolls = {
        .acc = (uint8_t)(furi_hal_random_get() % 100),
        .var = (uint8_t)(furi_hal_random_get() % 100),
        .crit = (uint8_t)(furi_hal_random_get() & 0xFF),
        .fx = (uint8_t)(furi_hal_random_get() % 100),
        .para = (uint8_t)(furi_hal_random_get() % 100),
        .miss = (uint8_t)(furi_hal_random_get() % 100),
    };
    bool bonus = side == DigiviceSidePlayer && app->crit_bonus;
    app->step = digivice_duel_act(&app->duel, side, app->turn_slot[side], bonus, &rolls);
    if(side == DigiviceSidePlayer) app->crit_bonus = false;
    step_messages(app);
}

static uint32_t step_frames(const DigiviceApp* app) {
    if(is_item_step(app)) return STEP_ITEM_FRAMES;
    uint32_t start = app->step.result == DigiviceStepSkipped ? STEP_SHOT : STEP_MSGS;
    return start + MSG_FRAMES * app->msg_count;
}

static void hp_snap(DigiviceApp* app) {
    for(uint8_t s = 0; s < DigiviceSideCount; s++) app->hp_shown[s] = app->duel.f[s].hp;
}

static void hp_ease(DigiviceApp* app) {
    for(uint8_t s = 0; s < DigiviceSideCount; s++) {
        uint16_t shown = app->hp_shown[s];
        uint16_t hp = app->duel.f[s].hp;
        uint16_t gap = shown > hp ? (uint16_t)(shown - hp) : (uint16_t)(hp - shown);
        uint16_t delta = gap / 4 > 1 ? (uint16_t)(gap / 4) : 1;
        if(shown > hp) {
            app->hp_shown[s] = (uint16_t)(shown - delta);
        } else if(shown < hp) {
            app->hp_shown[s] = (uint16_t)(shown + delta);
        }
    }
}

static bool battle_check_outcome(DigiviceApp* app) {
    DigiviceBattleOutcome outcome = digivice_duel_outcome(&app->duel);
    if(outcome == DigiviceBattleWon) {
        battle_finish(app, DigiviceBattleResultWin);
    } else if(outcome == DigiviceBattleLost) {
        battle_finish(app, DigiviceBattleResultLose);
    } else if(outcome == DigiviceBattleDrawn) {
        battle_finish(app, DigiviceBattleResultDraw);
    } else {
        return false;
    }
    return true;
}

static void step_end(DigiviceApp* app) {
    hp_snap(app);
    if(battle_check_outcome(app)) return;
    if(app->turn_done == 0) {
        app->turn_done = 1;
        step_start(app, (uint8_t)(1 - app->turn_first));
        return;
    }
    digivice_duel_end_round(&app->duel);
    if(battle_check_outcome(app)) return;
    battle_set_phase(app, DigiviceBattlePhaseMenu);
}

static void battle_commit(DigiviceApp* app, uint8_t pet_slot, bool item) {
    DigiviceDuel* d = &app->duel;
    notification_message(app->notification, &sequence_reset_rgb);
    uint8_t foe_slot = digivice_fighter_pick(
        &d->f[DigiviceSideFoe], &d->f[DigiviceSidePlayer], (uint8_t)furi_hal_random_get());
    app->turn_slot[DigiviceSidePlayer] = pet_slot;
    app->turn_slot[DigiviceSideFoe] = foe_slot;
    app->turn_item = item;
    app->turn_first = item || digivice_duel_player_first(d, pet_slot, foe_slot) ?
                          DigiviceSidePlayer :
                          DigiviceSideFoe;
    app->turn_done = 0;
    battle_set_phase(app, DigiviceBattlePhaseTurn);
    step_start(app, app->turn_first);
}

static void battle_lock_timing(DigiviceApp* app) {
    app->charge_locked = true;
    app->crit_bonus =
        digivice_rules_charge_tier(app->cursor, app->zone_start, app->zone_w) == DigiviceChargeBig;
    app->screen_frame = 0;
}

static void battle_try_retreat(DigiviceApp* app) {
    if(digivice_battle_retreat_allowed(&app->state, (DigiviceEncounterKind)app->enc_kind)) {
        battle_finish(app, DigiviceBattleResultFled);
    } else {
        digivice_app_notice(app, DigiviceStrBtlNoEscape);
    }
}

static void menu_confirm(DigiviceApp* app) {
    if(app->menu_sel == 0) {
        if(digivice_fighter_spent(&app->duel.f[DigiviceSidePlayer])) {
            battle_commit(app, DIGIVICE_SLOT_STRUGGLE, false);
        } else {
            battle_set_phase(app, DigiviceBattlePhaseMoves);
        }
    } else if(app->menu_sel == 1) {
        if(shield_available(app)) {
            battle_commit(app, DIGIVICE_SLOT_STRUGGLE, true);
        } else {
            digivice_app_notice(app, DigiviceStrBtlNoItem);
        }
    } else {
        battle_try_retreat(app);
    }
}

static void moves_confirm(DigiviceApp* app) {
    const DigiviceFighter* pet = &app->duel.f[DigiviceSidePlayer];
    if(pet->pp[app->move_sel] == 0) {
        digivice_app_notice(app, DigiviceStrBtlNoPp);
    } else if(digivice_move(pet->moves[app->move_sel])->power) {
        battle_set_phase(app, DigiviceBattlePhaseTiming);
    } else {
        battle_commit(app, app->move_sel, false);
    }
}

// The next stage boundary of the step on screen: OK skips to it.
static uint32_t step_boundary(uint32_t f) {
    if(f < STEP_SHOT) return STEP_SHOT;
    if(f < STEP_HIT) return STEP_HIT;
    if(f < STEP_MSGS) return STEP_MSGS;
    return STEP_MSGS + (f - STEP_MSGS) / MSG_FRAMES * MSG_FRAMES + MSG_FRAMES;
}

void digivice_battle_input(DigiviceApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) return;
    InputKey key = event->key;
    switch(app->phase) {
    case DigiviceBattlePhaseMenu:
        if(key == InputKeyUp) {
            app->menu_sel = (uint8_t)((app->menu_sel + 2) % 3);
        } else if(key == InputKeyDown) {
            app->menu_sel = (uint8_t)((app->menu_sel + 1) % 3);
        } else if(key == InputKeyOk) {
            menu_confirm(app);
        } else if(key == InputKeyBack) {
            battle_try_retreat(app);
        }
        break;
    case DigiviceBattlePhaseMoves:
        if(key == InputKeyLeft || key == InputKeyRight) {
            app->move_sel ^= 1;
        } else if(key == InputKeyUp || key == InputKeyDown) {
            app->move_sel ^= 2;
        } else if(key == InputKeyOk) {
            moves_confirm(app);
        } else if(key == InputKeyBack) {
            battle_set_phase(app, DigiviceBattlePhaseMenu);
        }
        break;
    case DigiviceBattlePhaseTiming:
        if(app->charge_locked) break;
        if(key == InputKeyOk) {
            battle_lock_timing(app);
        } else if(key == InputKeyBack) {
            battle_set_phase(app, DigiviceBattlePhaseMoves);
        }
        break;
    case DigiviceBattlePhaseTurn:
        // The tick adds the frame back, so the boundary's own events still fire.
        if(key == InputKeyOk) app->screen_frame = step_boundary(app->screen_frame) - 1;
        break;
    case DigiviceBattlePhaseEnd:
        if(app->screen_frame <
           (capture_active(app) ? DIGIVICE_CAPTURE_FRAMES : END_INPUT_FRAMES))
            break;
        if(key == InputKeyOk || key == InputKeyBack) battle_exit(app);
        break;
    }
}

void digivice_battle_tick(DigiviceApp* app) {
    uint32_t f = app->screen_frame;
    switch(app->phase) {
    case DigiviceBattlePhaseTiming:
        if(!app->charge_locked) {
            app->cursor = digivice_rules_meter_cursor(f);
            if(f >= DIGIVICE_CHARGE_FRAMES) battle_lock_timing(app);
        } else if(f >= LOCK_FRAMES) {
            battle_commit(app, app->move_sel, false);
        }
        break;
    case DigiviceBattlePhaseTurn:
        if(f == STEP_HIT && app->step.damage) {
            notification_message(app->notification, &digivice_sequence_hit);
        }
        if(f >= STEP_HIT) hp_ease(app);
        if(f >= step_frames(app)) step_end(app);
        break;
    case DigiviceBattlePhaseEnd:
        if(capture_active(app) && f == CAPTURE_RING_END &&
           app->reward.capture == DigiviceCaptureCaught) {
            notification_message(app->notification, &digivice_sequence_evolution);
        }
        if(app->area_cleared && f >= CLEAR_START + FADE_FRAMES) battle_exit(app);
        break;
    default:
        break;
    }
}

static void draw_end_bar(Canvas* canvas, DigiviceApp* app) {
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, 128, 8);
    canvas_set_color(canvas, ColorWhite);
    const char* foe = foe_name(app);
    digivice_draw_text(canvas, 2, 1, digivice_form_name((DigiviceForm)app->state.form));
    digivice_draw_text(canvas, 126 - digivice_draw_text_width(canvas, foe), 1, foe);
    digivice_draw_text(canvas, 58, 1, "VS");
    canvas_set_color(canvas, ColorBlack);
}

static void draw_capture(Canvas* canvas, DigiviceApp* app, uint32_t f) {
    int32_t cy = FEET_Y - 14;
    bool caught = app->reward.capture == DigiviceCaptureCaught;
    canvas_set_color(canvas, ColorBlack);
    if(f < CAPTURE_DIZZY_END) {
        digivice_draw_sparkle_ring(canvas, FOE_X, cy - 10, 10, 3, app->frame);
    } else if(f < CAPTURE_RING_END) {
        int32_t r = 22 - (int32_t)(f - CAPTURE_DIZZY_END) * 14 /
                             (CAPTURE_RING_END - CAPTURE_DIZZY_END);
        digivice_draw_dash_ring(canvas, FOE_X, cy, r, app->frame);
    } else {
        if(f == CAPTURE_RING_END) digivice_draw_invert(canvas, 0, 0, 128, 64);
        if(caught) {
            digivice_draw_sparkle_ring(canvas, FOE_X, cy, 14, 10, app->frame);
            digivice_draw_star5(canvas, FOE_X, cy);
        }
        digivice_draw_label(
            canvas, 10, digivice_app_str(app, caught ? DigiviceStrCaptured : DigiviceStrEscaped));
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
        lost ? DigivicePoseAttack :
               (capture_active(app) && f > CAPTURE_RING_END ? DigivicePoseIdle :
                                                              DigivicePoseHurt));
    if(won && capture_active(app)) {
        draw_capture(canvas, app, f);
    } else if(won) {
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
        } else if(app->reward.capture == DigiviceCaptureCaught && f >= CAPTURE_RING_END) {
            digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrCaptureNew));
        } else if(app->reward.capture == DigiviceCaptureFriend) {
            digivice_draw_text(canvas, 3, 59, digivice_app_str(app, DigiviceStrCaptureFriend));
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

static int32_t foe_feet(const DigiviceApp* app) {
    return app->enc_kind == DigiviceEncounterBoss ? ARENA_BOSS_FEET : ARENA_FOE_FEET;
}

static int32_t mouth_x(uint8_t side) {
    return side == DigiviceSidePlayer ? 32 : 88;
}

static int32_t mouth_y(uint8_t side) {
    return side == DigiviceSidePlayer ? 27 : 12;
}

static int32_t side_x(uint8_t side) {
    return side == DigiviceSidePlayer ? ARENA_PET_X : ARENA_FOE_X;
}

// Top-left of the guard shield beside a fighter.
static int32_t shield_x(uint8_t side) {
    return side == DigiviceSidePlayer ? ARENA_PET_X + 14 : ARENA_FOE_X - 20;
}

static int32_t shield_y(uint8_t side) {
    return side == DigiviceSidePlayer ? 22 : 8;
}

static void draw_arena(
    Canvas* canvas,
    DigiviceApp* app,
    int32_t ox,
    int32_t oy,
    const int32_t dy[DigiviceSideCount],
    const DigivicePose pose[DigiviceSideCount]) {
    canvas_set_color(canvas, ColorBlack);
    for(int32_t y = 3; y < 41; y += 8)
        for(int32_t x = 4; x < 128; x += 8)
            canvas_draw_dot(canvas, x + ox, y + oy);
    int32_t fy = foe_feet(app);
    digivice_draw_ring(canvas, ARENA_PET_X + ox, ARENA_PET_FEET + 1 + oy, 14, 2);
    digivice_draw_ring(canvas, ARENA_FOE_X + ox, fy + 1 + oy, 16, 2);
    // The pet has no hurt pose: its hit shows in the shake and the flash.
    DigivicePose pet_pose = pose[DigiviceSidePlayer] == DigivicePoseHurt ?
                                DigivicePoseIdle :
                                pose[DigiviceSidePlayer];
    digivice_draw_form(
        canvas,
        (DigiviceForm)app->state.form,
        ARENA_PET_X + ox,
        ARENA_PET_CY + oy + dy[DigiviceSidePlayer],
        app->frame,
        pet_pose,
        false);
    draw_foe_sprite(
        canvas,
        app,
        ARENA_FOE_X + ox,
        fy + oy + dy[DigiviceSideFoe],
        pose[DigiviceSideFoe]);
}

// Guard, glitch and pending charge of both fighters, except the one whose move is on screen.
static void draw_standing(Canvas* canvas, DigiviceApp* app) {
    bool playing = app->phase == DigiviceBattlePhaseTurn && app->screen_frame < STEP_MSGS;
    for(uint8_t s = 0; s < DigiviceSideCount; s++) {
        const DigiviceFighter* f = &app->duel.f[s];
        bool fresh = playing && app->step.actor == s;
        bool guard = f->guard || (s == DigiviceSidePlayer && app->duel.shield_armed);
        if(guard && !(fresh && app->step.fx == DigiviceFxShield)) {
            digivice_draw_shield(canvas, shield_x(s), shield_y(s));
        }
        if(f->charged && !(fresh && app->step.fx == DigiviceFxCharge)) {
            digivice_draw_dash_ring(canvas, mouth_x(s), mouth_y(s), 6, app->frame);
        }
        if(f->glitch && app->frame % 4 == 0) digivice_draw_glitch(canvas, app->frame + s, 2);
    }
}

static void draw_shot_lines(Canvas* canvas, int32_t x, int32_t y, int8_t dir) {
    for(int32_t k = 1; k <= 3; k++) {
        int32_t ly = y - 4 + k * 2;
        digivice_draw_line(canvas, x - dir * (6 + k * 4), ly, x - dir * (2 + k * 4), ly);
    }
}

static void draw_impact(Canvas* canvas, const DigiviceStep* step, uint8_t target, int32_t t) {
    int32_t tx = mouth_x(target);
    int32_t ty = mouth_y(target);
    if(step->result == DigiviceStepShielded) {
        digivice_draw_shield(canvas, shield_x(target), shield_y(target));
        digivice_draw_burst(canvas, tx, ty, 4 + t * 2);
        return;
    }
    if(t < 2) {
        if(target == DigiviceSidePlayer) {
            digivice_draw_invert(canvas, ARENA_PET_X - 16, 14, 32, 28);
        } else {
            digivice_draw_invert(canvas, ARENA_FOE_X - 16, 0, 32, 28);
        }
    }
    digivice_draw_star5(canvas, tx, ty);
    digivice_draw_burst(canvas, tx, ty, 4 + t * 2);
    // "-N" floats up from the target in a small white box.
    char text[8];
    snprintf(text, sizeof(text), "-%u", (unsigned)step->damage);
    int32_t w = digivice_draw_text_width(canvas, text) + 2;
    int32_t ny = ty - 2 - t * 2;
    if(ny < 1) ny = 1;
    canvas_set_color(canvas, ColorWhite);
    digivice_draw_box(canvas, tx - w / 2, ny - 1, w, 8);
    canvas_set_color(canvas, ColorBlack);
    digivice_draw_text(canvas, tx - w / 2 + 1, ny, text);
}

// Effect glyph of a status move, frames STEP_SHOT .. STEP_MSGS - 1.
static void draw_status_fx(Canvas* canvas, DigiviceApp* app, uint32_t f) {
    const DigiviceStep* step = &app->step;
    uint8_t user = step->actor;
    uint8_t target = (uint8_t)(1 - user);
    int32_t t = (int32_t)(f - STEP_SHOT);
    switch(step->fx) {
    case DigiviceFxHeal:
        for(int32_t i = 0; i < 3; i++) {
            digivice_draw_heart(
                canvas, side_x(user) - 10 + i * 7, mouth_y(user) + 6 - t - (i % 2) * 3);
        }
        break;
    case DigiviceFxShield:
        digivice_draw_shield(canvas, shield_x(user), shield_y(user));
        break;
    case DigiviceFxCharge:
        digivice_draw_dash_ring(canvas, mouth_x(user), mouth_y(user), 4 + t, app->frame);
        break;
    case DigiviceFxAtkDown:
    case DigiviceFxDefDown:
        for(int32_t i = 0; i < 2; i++) {
            digivice_draw_arrow(
                canvas, side_x(target) - 8 + i * 8, mouth_y(target) - 8 + t, false, true);
        }
        break;
    case DigiviceFxGlitch:
        digivice_draw_glitch(canvas, app->frame, 4);
        break;
    default:
        break;
    }
}

// The step on screen: poses, shake, projectile, impact and effects; draws the arena too.
static void draw_turn_scene(Canvas* canvas, DigiviceApp* app) {
    static const int8_t hop[5] = {-4, -8, -10, -8, -4};
    const DigiviceStep* step = &app->step;
    uint32_t f = app->screen_frame;
    bool item = is_item_step(app);
    bool skipped = step->result == DigiviceStepSkipped;
    const DigiviceMove* move = digivice_move(step->move);
    bool attack = !item && !skipped && move->power > 0;
    uint8_t actor = step->actor;
    uint8_t target = (uint8_t)(1 - actor);
    int32_t t = (int32_t)f - STEP_HIT;
    bool impact = t >= 0 && t < STEP_MSGS - STEP_HIT;
    bool hit = attack && step->damage > 0;

    int32_t ox = 0;
    int32_t oy = 0;
    if(hit && t >= 0 && t < 4) {
        int32_t amp = t == 0 ? 2 : 1;
        ox = (app->frame % 2) ? amp : -amp;
        oy = (app->frame % 3) ? 0 : amp;
    }
    int32_t dy[DigiviceSideCount] = {0, 0};
    DigivicePose pose[DigiviceSideCount] = {DigivicePoseIdle, DigivicePoseIdle};
    if(attack && f < STEP_HIT) pose[actor] = DigivicePoseAttack;
    if(hit && impact) pose[target] = DigivicePoseHurt;
    if(attack && step->result == DigiviceStepMissed && impact) dy[target] = hop[t];
    draw_arena(canvas, app, ox, oy, dy, pose);
    draw_standing(canvas, app);

    if(item) {
        digivice_draw_shield(canvas, shield_x(DigiviceSidePlayer), shield_y(DigiviceSidePlayer));
    } else if(skipped) {
        if(f < STEP_SHOT) digivice_draw_glitch(canvas, app->frame, 3);
    } else if(attack) {
        int32_t ax = mouth_x(actor);
        int32_t ay = mouth_y(actor);
        int32_t bx = mouth_x(target);
        int32_t by = mouth_y(target);
        int8_t dir = actor == DigiviceSidePlayer ? 1 : -1;
        uint8_t size = app->duel.f[actor].shot;
        if(f >= STEP_RING && f < STEP_SHOT) {
            digivice_draw_dash_ring(canvas, ax, ay, 4 + (int32_t)(f - STEP_RING), app->frame);
        } else if(f >= STEP_SHOT && f < STEP_HIT) {
            int32_t k = (int32_t)(f - STEP_SHOT);
            digivice_draw_shot(
                canvas,
                (DigiviceAttr)move->attr,
                ax + (bx - ax) * k / (STEP_HIT - STEP_SHOT),
                ay + (by - ay) * k / (STEP_HIT - STEP_SHOT),
                size,
                dir,
                app->frame);
        } else if(impact && step->result == DigiviceStepMissed) {
            // The shot flies on past the target with speed lines.
            int32_t x = bx + dir * (t + 1) * 14;
            digivice_draw_shot(canvas, (DigiviceAttr)move->attr, x, by, size, dir, app->frame);
            draw_shot_lines(canvas, x, by, dir);
        } else if(impact) {
            draw_impact(canvas, step, target, t);
        }
    } else if(f >= STEP_SHOT && f < STEP_MSGS) {
        draw_status_fx(canvas, app, f);
    }
}

static void draw_hud_box(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    const DigiviceFighter* fighter,
    uint16_t shown,
    const char* name) {
    char text[24];
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, x, y, 80, 14);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_frame(canvas, x, y, 80, 14);
    digivice_draw_attr_icon(canvas, (DigiviceAttr)fighter->attr, x + 3, y + 2);
    size_t n = utf8_prefix(name, 9);
    if(n > sizeof(text) - 1) n = sizeof(text) - 1;
    memcpy(text, name, n);
    text[n] = '\0';
    digivice_draw_text(canvas, x + 12, y + 2, text);
    snprintf(text, sizeof(text), "LV%u", (unsigned)fighter->level);
    digivice_draw_text(canvas, x + 78 - digivice_draw_text_width(canvas, text), y + 2, text);
    canvas_draw_frame(canvas, x + 3, y + 9, 44, 4);
    int32_t fill = fighter->hp_max ? (int32_t)(44UL * shown / fighter->hp_max) : 0;
    if(fill < 1 && shown > 0) fill = 1;
    digivice_draw_box(canvas, x + 3, y + 9, fill, 4);
    snprintf(text, sizeof(text), "%u/%u", (unsigned)shown, (unsigned)fighter->hp_max);
    digivice_draw_text(canvas, x + 50, y + 8, text);
}

static void draw_menu(Canvas* canvas, DigiviceApp* app) {
    static const DigiviceStr rows[3] = {
        DigiviceStrBtlFight,
        DigiviceStrBtlItem,
        DigiviceStrBtlRun,
    };
    char line[40];
    digivice_draw_text(canvas, 3, 45, digivice_app_str(app, DigiviceStrBtlWhat));
    snprintf(
        line,
        sizeof(line),
        "%s%s",
        digivice_form_name((DigiviceForm)app->state.form),
        digivice_app_str(app, DigiviceStrBtlWhatPost));
    digivice_draw_text(canvas, 3, 53, line);
    for(int32_t i = 0; i < 3; i++) {
        int32_t y = 45 + i * 6;
        if(i == app->menu_sel) {
            digivice_draw_box(canvas, 72, y - 1, 56, 7);
            canvas_set_color(canvas, ColorWhite);
        }
        digivice_draw_text(canvas, 78, y, digivice_app_str(app, rows[i]));
        canvas_set_color(canvas, ColorBlack);
    }
}

static void draw_moves(Canvas* canvas, DigiviceApp* app) {
    const DigiviceFighter* pet = &app->duel.f[DigiviceSidePlayer];
    char line[48];
    for(int32_t i = 0; i < DIGIVICE_SLOTS; i++) {
        const DigiviceMove* move = digivice_move(pet->moves[i]);
        int32_t x = (i & 1) * 64;
        int32_t y = 45 + (i >> 1) * 6;
        digivice_draw_attr_icon(canvas, (DigiviceAttr)move->attr, x, y - 1);
        digivice_draw_text(canvas, x + 7, y, move->name);
        if(pet->pp[i]) {
            snprintf(line, sizeof(line), "%02u", (unsigned)pet->pp[i]);
        } else {
            snprintf(line, sizeof(line), "--");
        }
        digivice_draw_text(canvas, x + 56, y, line);
        if(i == app->move_sel) digivice_draw_invert(canvas, x, y - 1, 63, 7);
    }
    const DigiviceMove* sel = digivice_move(pet->moves[app->move_sel]);
    char power[8] = "--";
    char acc[8] = "--";
    if(sel->power) snprintf(power, sizeof(power), "%u", (unsigned)sel->power);
    if(sel->acc) snprintf(acc, sizeof(acc), "%u", (unsigned)sel->acc);
    snprintf(
        line,
        sizeof(line),
        "%s%s %s%s PP%u/%u",
        digivice_app_str(app, DigiviceStrBtlPow),
        power,
        digivice_app_str(app, DigiviceStrBtlAcc),
        acc,
        (unsigned)pet->pp[app->move_sel],
        (unsigned)sel->pp);
    digivice_draw_text(canvas, 3, 58, line);
}

static void draw_turn_panel(Canvas* canvas, DigiviceApp* app) {
    const DigiviceStep* step = &app->step;
    uint32_t f = app->screen_frame;
    char line[48];
    if(is_item_step(app)) {
        digivice_draw_text(canvas, 3, 45, digivice_app_str(app, DigiviceStrBtlShieldReady));
        return;
    }
    uint32_t start = step->result == DigiviceStepSkipped ? STEP_SHOT : STEP_MSGS;
    if(f < start) {
        if(step->result == DigiviceStepSkipped) return;
        snprintf(
            line,
            sizeof(line),
            "%s%s",
            step->actor == DigiviceSidePlayer ? digivice_form_name((DigiviceForm)app->state.form) :
                                                foe_name(app),
            digivice_app_str(app, DigiviceStrBtlUsed));
        digivice_draw_text(canvas, 3, 45, line);
        snprintf(line, sizeof(line), "%s!", digivice_move(step->move)->name);
        digivice_draw_text(canvas, 3, 53, line);
        return;
    }
    uint32_t i = (f - start) / MSG_FRAMES;
    if(i < app->msg_count) {
        digivice_draw_text(canvas, 3, 45, digivice_app_str(app, app->msgs[i]));
    }
}

void digivice_battle_draw(Canvas* canvas, DigiviceApp* app) {
    if(app->phase == DigiviceBattlePhaseEnd) {
        draw_end(canvas, app);
        draw_end_bar(canvas, app);
        return;
    }
    if(app->phase == DigiviceBattlePhaseTurn) {
        draw_turn_scene(canvas, app);
    } else {
        static const int32_t still[DigiviceSideCount] = {0, 0};
        DigivicePose pose[DigiviceSideCount] = {DigivicePoseIdle, DigivicePoseIdle};
        if(app->phase == DigiviceBattlePhaseTiming && app->charge_locked) {
            pose[DigiviceSidePlayer] = DigivicePoseAttack;
        }
        draw_arena(canvas, app, 0, 0, still, pose);
        draw_standing(canvas, app);
    }

    const DigiviceDuel* d = &app->duel;
    draw_hud_box(
        canvas,
        0,
        0,
        &d->f[DigiviceSideFoe],
        app->hp_shown[DigiviceSideFoe],
        foe_name(app));
    draw_hud_box(
        canvas,
        48,
        28,
        &d->f[DigiviceSidePlayer],
        app->hp_shown[DigiviceSidePlayer],
        digivice_form_name((DigiviceForm)app->state.form));

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 43, 128, 21);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, 42, 127, 42);
    switch(app->phase) {
    case DigiviceBattlePhaseMenu:
        draw_menu(canvas, app);
        break;
    case DigiviceBattlePhaseMoves:
        draw_moves(canvas, app);
        break;
    case DigiviceBattlePhaseTiming:
        digivice_draw_text(canvas, 3, 45, digivice_app_str(app, DigiviceStrBtlCritHint));
        digivice_draw_charge_meter(canvas, 3, 51, app->zone_start, app->zone_w, app->cursor);
        digivice_draw_ok_button(canvas, 109, 45, app->charge_locked);
        break;
    case DigiviceBattlePhaseTurn:
        draw_turn_panel(canvas, app);
        break;
    default:
        break;
    }
    if(app->notice_frames) digivice_draw_label(canvas, 20, digivice_app_str(app, app->notice));
}
