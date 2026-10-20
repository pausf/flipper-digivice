// Host-side test for the battle rules. Run: applications/services/digivice/test/run.sh
#include "../digivice_battle.h"
#include "../digivice_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                \
    do {                                                           \
        if(!(cond)) {                                              \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                            \
        }                                                          \
    } while(0)

#define H(n)   ((uint32_t)(n) * 3600UL)
#define DAY(n) ((uint32_t)(n) * 86400UL)

// Day 10 at 08:00, a wide-awake starting point.
#define T0 (DAY(10) + H(8))

static DigiviceState with_form(uint8_t form) {
    DigiviceState s;
    digivice_rules_init(&s, T0);
    s.form = form;
    return s;
}

// A battle with fixed powers and attributes, as the hit-chance table in the spec.
static DigiviceBattle duel(uint16_t p, uint8_t pa, uint16_t q, uint8_t qa) {
    DigiviceBattle b;
    memset(&b, 0, sizeof(b));
    b.player_power = p;
    b.player_attr = pa;
    b.foe_power = q;
    b.foe_attr = qa;
    return b;
}

static void test_hit_pct(void) {
    CHECK(digivice_rules_hit_pct(20, 10, 0) == 67);
    CHECK(digivice_rules_hit_pct(30, 10, 0) == 75);
    CHECK(digivice_rules_hit_pct(64, 59, 1) == 57);
    CHECK(digivice_rules_hit_pct(59, 64, -1) == 43);
    CHECK(digivice_rules_hit_pct(64, 131, 0) == 33);
    CHECK(digivice_rules_hit_pct(50, 50, 1) == 55);
    CHECK(digivice_rules_hit_pct(50, 50, -1) == 45);
    CHECK(digivice_rules_hit_pct(100, 1, 0) == DIGIVICE_HIT_MAX_PCT);
    CHECK(digivice_rules_hit_pct(100, 1, 1) == DIGIVICE_HIT_MAX_PCT);
    CHECK(digivice_rules_hit_pct(1, 100, 0) == DIGIVICE_HIT_MIN_PCT);
    CHECK(digivice_rules_hit_pct(1, 100, -1) == DIGIVICE_HIT_MIN_PCT);
    CHECK(digivice_rules_hit_pct(0, 0, 0) == 50);
}

static void test_power(void) {
    DigiviceState s = with_form(DigiviceFormByteegg);
    CHECK(digivice_rules_player_power(&s, T0) == 0);
    s.form = DigiviceFormBitling;
    CHECK(digivice_rules_player_power(&s, T0) == 10);
    s.form = DigiviceFormBytemon;
    CHECK(digivice_rules_player_power(&s, T0) == 20);
    s.form = DigiviceFormWavedrake;
    CHECK(digivice_rules_player_power(&s, T0) == 40);
    s.strength = 24;
    CHECK(digivice_rules_player_power(&s, T0) == 64);
    s.strength = 300;
    CHECK(digivice_rules_player_power(&s, T0) == 64);
    s.injured_until = T0 + 1;
    CHECK(digivice_rules_injured(&s, T0));
    CHECK(digivice_rules_player_power(&s, T0) == 48);
    CHECK(!digivice_rules_injured(&s, T0 + 1));
    CHECK(digivice_rules_player_power(&s, T0 + 1) == 64);
    // A corrupt Champion keeps its stage power.
    s.injured_until = 0;
    s.form = DigiviceFormVirubyte;
    s.prev_form = DigiviceFormTagdrake;
    CHECK(digivice_rules_player_power(&s, T0) == 64);

    CHECK(digivice_rules_charged_power(20, DigiviceChargeSmall) == 12);
    CHECK(digivice_rules_charged_power(20, DigiviceChargeNormal) == 20);
    CHECK(digivice_rules_charged_power(20, DigiviceChargeBig) == 30);
}

static void test_charge(void) {
    CHECK(digivice_rules_charge_zone_w(0) == 3);
    CHECK(digivice_rules_charge_zone_w(8) == 4);
    CHECK(digivice_rules_charge_zone_w(16) == 5);
    CHECK(digivice_rules_charge_zone_w(24) == 6);
    CHECK(digivice_rules_charge_zone_w(1000) == 6);
    for(uint8_t w = 3; w <= 6; w++) {
        bool low = false, high = false;
        for(uint32_t r = 0; r < 256; r++) {
            uint8_t start = digivice_rules_charge_zone_start(w, (uint8_t)r);
            CHECK(start >= 3 && start <= DIGIVICE_METER_SEGMENTS - w - 1);
            low |= start == 3;
            high |= start == DIGIVICE_METER_SEGMENTS - w - 1;
        }
        CHECK(low && high);
    }
    // Zone 8..11: inside is Big, two segments either side Normal, beyond Small.
    for(uint8_t c = 8; c <= 11; c++)
        CHECK(digivice_rules_charge_tier(c, 8, 4) == DigiviceChargeBig);
    CHECK(digivice_rules_charge_tier(7, 8, 4) == DigiviceChargeNormal);
    CHECK(digivice_rules_charge_tier(6, 8, 4) == DigiviceChargeNormal);
    CHECK(digivice_rules_charge_tier(12, 8, 4) == DigiviceChargeNormal);
    CHECK(digivice_rules_charge_tier(13, 8, 4) == DigiviceChargeNormal);
    CHECK(digivice_rules_charge_tier(5, 8, 4) == DigiviceChargeSmall);
    CHECK(digivice_rules_charge_tier(14, 8, 4) == DigiviceChargeSmall);
    CHECK(digivice_rules_charge_tier(0, 3, 3) == DigiviceChargeSmall);
    CHECK(digivice_rules_charge_tier(1, 3, 3) == DigiviceChargeNormal);

    // The cursor ping-pongs; a timeout at frame 30 stops it wherever it is.
    CHECK(digivice_rules_meter_cursor(0) == 0);
    CHECK(digivice_rules_meter_cursor(19) == 19);
    CHECK(digivice_rules_meter_cursor(20) == 18);
    CHECK(digivice_rules_meter_cursor(38) == 0);
    CHECK(digivice_rules_meter_cursor(DIGIVICE_CHARGE_FRAMES) == 8);
}

static void test_guard(void) {
    static const uint32_t frames[5] = {0, 4, 5, 9, 10};
    static const uint8_t from_high[5] = {0, 0, 1, 1, 0};
    for(int i = 0; i < 5; i++) {
        CHECK(digivice_rules_guard_at(DigiviceAimHigh, frames[i]) == from_high[i]);
        CHECK(digivice_rules_guard_at(DigiviceAimLow, frames[i]) == !from_high[i]);
    }
    // Sampled 5 frames after the press: aiming high at frame 0 meets the low guard.
    CHECK(!digivice_rules_shot_blocked(DigiviceAimHigh, DigiviceAimHigh, 0));
    CHECK(digivice_rules_shot_blocked(DigiviceAimLow, DigiviceAimHigh, 0));
    CHECK(digivice_rules_shot_blocked(DigiviceAimHigh, DigiviceAimHigh, 5));
    CHECK(!digivice_rules_shot_blocked(DigiviceAimHigh, DigiviceAimHigh, 4));
}

static void test_shot_outcomes(void) {
    // P 20 vs Q 10: 67% unblocked, 33% blocked.
    DigiviceBattle b = duel(20, DigiviceAttrNulo, 10, DigiviceAttrNulo);
    CHECK(digivice_battle_player_shot(&b, DigiviceChargeNormal, false, 99, 66) ==
          DigiviceShotHit);
    CHECK(digivice_battle_player_shot(&b, DigiviceChargeNormal, false, 99, 67) ==
          DigiviceShotDodge);
    CHECK(digivice_battle_player_shot(&b, DigiviceChargeNormal, true, 99, 32) ==
          DigiviceShotGuardBroken);
    CHECK(digivice_battle_player_shot(&b, DigiviceChargeNormal, true, 99, 33) ==
          DigiviceShotBlocked);
    CHECK(b.player_hits == 2);
    // Big charge: P 30, 75%.
    CHECK(digivice_battle_player_shot(&b, DigiviceChargeBig, false, 99, 74) == DigiviceShotHit);

    // A blocked shot never drops below 5%.
    DigiviceBattle weak = duel(1, DigiviceAttrNulo, 100, DigiviceAttrNulo);
    CHECK(digivice_battle_player_shot(&weak, DigiviceChargeNormal, true, 99, 4) ==
          DigiviceShotGuardBroken);
    CHECK(digivice_battle_player_shot(&weak, DigiviceChargeNormal, true, 99, 5) ==
          DigiviceShotBlocked);

    // A glitchy pet first rolls the 25% corruption miss.
    DigiviceBattle g = duel(100, DigiviceAttrNulo, 1, DigiviceAttrNulo);
    g.glitchy = true;
    CHECK(digivice_battle_player_shot(&g, DigiviceChargeNormal, false, 24, 0) ==
          DigiviceShotDodge);
    CHECK(digivice_battle_player_shot(&g, DigiviceChargeNormal, false, 25, 0) ==
          DigiviceShotHit);
    CHECK(digivice_battle_player_shot(&g, DigiviceChargeNormal, true, 24, 0) ==
          DigiviceShotBlocked);
}

static void test_foe_turn_symmetry(void) {
    // Wavedrake (Portador, 64) against a Ruido foe of 59: 57% one way, 43% the other.
    DigiviceBattle b = duel(64, DigiviceAttrPortador, 59, DigiviceAttrRuido);
    CHECK(digivice_battle_player_shot(&b, DigiviceChargeNormal, false, 99, 56) ==
          DigiviceShotHit);
    CHECK(digivice_battle_player_shot(&b, DigiviceChargeNormal, false, 99, 57) ==
          DigiviceShotDodge);
    CHECK(digivice_battle_foe_shot(&b, 42) == DigiviceShotHit);
    CHECK(digivice_battle_foe_shot(&b, 43) == DigiviceShotDodge);
    CHECK(b.foe_hits == 1 && b.round == 2);
    // The foe uses the uncharged power: a Big charge last turn changes nothing.
    digivice_battle_player_shot(&b, DigiviceChargeBig, false, 99, 99);
    CHECK(digivice_battle_foe_shot(&b, 43) == DigiviceShotDodge);
}

static DigiviceBattleOutcome play(DigiviceBattle* b, const char* script) {
    // 'p' player hit, 'f' foe hit, '-' a miss on that side, in turn order.
    bool player_turn = true;
    for(const char* c = script; *c; c++) {
        if(player_turn) {
            digivice_battle_player_shot(b, DigiviceChargeNormal, false, 99, *c == 'p' ? 0 : 99);
        } else {
            digivice_battle_foe_shot(b, *c == 'f' ? 0 : 99);
        }
        player_turn = !player_turn;
        if(digivice_battle_outcome(b) != DigiviceBattleOngoing) break;
    }
    return digivice_battle_outcome(b);
}

static void test_three_hits(void) {
    DigiviceBattle b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(play(&b, "p-p-p") == DigiviceBattleWon && b.player_hits == 3 && b.round == 2);

    b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(play(&b, "pfpfp") == DigiviceBattleWon && b.foe_hits == 2);

    b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(play(&b, "pfpf-f") == DigiviceBattleLost && b.player_hits == 2 && b.foe_hits == 3);

    // The battle ends at the shot that reaches 3: the foe never gets its turn.
    b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    play(&b, "p-p-p");
    CHECK(b.round == 2 && digivice_battle_outcome(&b) == DigiviceBattleWon);
}

static void test_round_cap(void) {
    DigiviceBattle b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(play(&b, "--------------------") == DigiviceBattleDrawn && b.round == 10);
    b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(play(&b, "p-------------------") == DigiviceBattleWon && b.round == 10);
    b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(play(&b, "pf-f----------------") == DigiviceBattleLost && b.round == 10);
    b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(play(&b, "pfpf----------------") == DigiviceBattleDrawn);
    b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(play(&b, "------------------") == DigiviceBattleOngoing && b.round == 9);
}

static void test_shield_cancels_first_hit(void) {
    DigiviceBattle b = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    b.shield = true;
    CHECK(digivice_battle_foe_shot(&b, 0) == DigiviceShotShielded);
    CHECK(b.foe_hits == 0 && !b.shield && b.shield_used);
    CHECK(digivice_battle_foe_shot(&b, 0) == DigiviceShotHit && b.foe_hits == 1);
    // A miss leaves the Shield up.
    DigiviceBattle c = duel(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    c.shield = true;
    CHECK(digivice_battle_foe_shot(&c, 99) == DigiviceShotDodge && c.shield);
}

static void test_battle_init(void) {
    DigiviceState s = with_form(DigiviceFormWavedrake);
    s.strength = 24;
    s.area = 4;
    s.enc_foe = 0;
    s.item = DigiviceItemShield;
    DigiviceBattle b;
    digivice_battle_init(&b, &s, DigiviceEncounterTrail, T0);
    CHECK(b.foe == DigiviceFoeFuzzcan && b.foe_attr == DigiviceAttrRuido);
    CHECK(b.foe_power == 59 && b.foe_shot == DIGIVICE_SHOT_CHAMP_FOE);
    CHECK(b.player_power == 64 && b.player_attr == DigiviceAttrPortador);
    CHECK(b.player_shot == 5 && b.shield && !b.glitchy && b.player_hits == 0);
    s.enc_foe = 1;
    digivice_battle_init(&b, &s, DigiviceEncounterTrail, T0);
    CHECK(b.foe == DigiviceFoeKorno && b.foe_power == 46 && b.foe_shot == 3);

    digivice_battle_init(&b, &s, DigiviceEncounterBoss, T0);
    CHECK(b.foe == 4 && b.foe_power == 64 && b.foe_shot == DIGIVICE_SHOT_BOSS);
    s.area = 7;
    digivice_battle_init(&b, &s, DigiviceEncounterBoss, T0);
    CHECK(b.foe_power == 131 && b.foe_shot == DIGIVICE_SHOT_BEAM);

    DigiviceState r = with_form(DigiviceFormBytemon);
    r.virux_type = DigiviceTypeSubGhz;
    r.corruption = 60;
    digivice_battle_init(&b, &r, DigiviceEncounterVirux, T0);
    CHECK(b.foe_power == 24 && b.foe_attr == DigiviceAttrPortador && b.foe_shot == 3);
    CHECK(b.player_shot == 3 && b.glitchy && !b.shield);
    r.form = DigiviceFormTagdrake;
    digivice_battle_init(&b, &r, DigiviceEncounterVirux, T0);
    CHECK(b.foe_power == 46 && b.foe_shot == 5);

    // The Baby fights trail foes with a 2x2 shot.
    DigiviceState baby = with_form(DigiviceFormBitling);
    baby.enc_foe = 0;
    digivice_battle_init(&b, &baby, DigiviceEncounterTrail, T0);
    CHECK(b.player_power == 10 && b.player_shot == 2 && b.foe_power == 10);
}

static void test_can_battle(void) {
    DigiviceState s = with_form(DigiviceFormBitling);
    CHECK(digivice_rules_can_battle(&s, T0) == DigiviceResultOk);
    CHECK(digivice_rules_can_battle(&s, DAY(10) + H(23) + 60) == DigiviceResultAsleep);
    s.sick = 1;
    CHECK(digivice_rules_can_battle(&s, DAY(10) + H(23) + 60) == DigiviceResultSick);
    DigiviceState egg = with_form(DigiviceFormByteegg);
    CHECK(digivice_rules_can_battle(&egg, T0) == DigiviceResultTooYoung);
}

static void test_retreat(void) {
    DigiviceState s = with_form(DigiviceFormTagdrake);
    for(uint8_t a = 0; a < DIGIVICE_AREA_COUNT; a++) {
        s.area = a;
        CHECK(digivice_battle_retreat_allowed(&s, DigiviceEncounterVirux));
        CHECK(digivice_battle_retreat_allowed(&s, DigiviceEncounterTrail) == (a < 4));
        CHECK(digivice_battle_retreat_allowed(&s, DigiviceEncounterBoss) == (a < 4));
    }
}

static DigiviceReward finish(
    DigiviceState* s,
    DigiviceEncounterKind kind,
    DigiviceBattleResult result,
    bool shield_used,
    uint8_t rng_pct) {
    return digivice_rules_battle_result(
        s, kind, result, shield_used, s->enc_expires, rng_pct, T0);
}

static void test_stale_fight_keeps_newer_encounter(void) {
    DigiviceState s = with_form(DigiviceFormTagdrake);
    s.enc_foe = 1;
    s.enc_expires = T0 + 500;
    uint32_t started_with = T0 + 100;
    digivice_rules_battle_result(
        &s, DigiviceEncounterTrail, DigiviceBattleResultWin, false, started_with, 99, T0);
    CHECK(s.enc_foe == 1 && s.enc_expires == T0 + 500);
    digivice_rules_battle_result(
        &s, DigiviceEncounterTrail, DigiviceBattleResultWin, false, T0 + 500, 99, T0);
    CHECK(s.enc_foe == DIGIVICE_ENC_NONE && s.enc_expires == 0);
}

static void test_trail_rewards(void) {
    for(uint8_t a = 0; a < DIGIVICE_AREA_COUNT; a++) {
        DigiviceState s = with_form(DigiviceFormTagdrake);
        s.area = a;
        s.enc_foe = 1;
        s.enc_expires = T0 + 60;
        DigiviceReward r = finish(&s, DigiviceEncounterTrail, DigiviceBattleResultWin, false, 99);
        CHECK(r.exp == 6 + 2 * (a + 1) && s.exp == r.exp && s.wins == 1);
        CHECK(r.item == DigiviceItemNone && s.enc_foe == DIGIVICE_ENC_NONE && s.enc_expires == 0);
    }
    DigiviceState d = with_form(DigiviceFormTagdrake);
    uint8_t drop = DIGIVICE_DROP_PCT - 1;
    DigiviceReward r = finish(&d, DigiviceEncounterTrail, DigiviceBattleResultWin, false, drop);
    CHECK(r.item == DigiviceItemShield && d.item == DigiviceItemShield);
    d.item = DigiviceItemNone;
    r = finish(&d, DigiviceEncounterTrail, DigiviceBattleResultWin, false, DIGIVICE_DROP_PCT);
    CHECK(r.item == DigiviceItemNone && d.item == DigiviceItemNone);
    // A held Shield is not stacked.
    d.item = DigiviceItemShield;
    r = finish(&d, DigiviceEncounterTrail, DigiviceBattleResultWin, false, 0);
    CHECK(r.item == DigiviceItemNone && d.item == DigiviceItemShield);
    // A Shield used in the fight is gone, and a drop can replace it.
    r = finish(&d, DigiviceEncounterTrail, DigiviceBattleResultWin, true, 99);
    CHECK(d.item == DigiviceItemNone);
    d.item = DigiviceItemShield;
    r = finish(&d, DigiviceEncounterTrail, DigiviceBattleResultWin, true, 0);
    CHECK(d.item == DigiviceItemShield && r.item == DigiviceItemShield);
}

static void test_boss_rewards(void) {
    for(uint8_t a = 0; a < DIGIVICE_AREA_COUNT; a++) {
        DigiviceState s = with_form(DigiviceFormTagdrake);
        s.cleared = (uint8_t)((1U << a) - 1);
        s.area = a;
        s.distance = 0;
        DigiviceReward r = finish(&s, DigiviceEncounterBoss, DigiviceBattleResultWin, false, 99);
        CHECK(r.exp == 40 + 20 * (a + 1) && r.item == DigiviceItemShield);
        CHECK(s.cleared & (1U << a));
    }
    // A repeat clear pays half.
    DigiviceState s = with_form(DigiviceFormTagdrake);
    s.cleared = 0x03;
    s.area = 0;
    s.distance = 0;
    s.item = DigiviceItemShield;
    DigiviceReward r = finish(&s, DigiviceEncounterBoss, DigiviceBattleResultWin, false, 99);
    CHECK(r.exp == 30 && r.item == DigiviceItemNone && s.area == 2 && s.distance == 720);
}

static void test_defeat_penalties(void) {
    for(uint8_t a = 0; a < DIGIVICE_AREA_COUNT; a++) {
        DigiviceState s = with_form(DigiviceFormTagdrake);
        s.area = a;
        s.distance = 100;
        s.mid_state = 2;
        s.mood = 3;
        s.enc_foe = 0;
        s.enc_expires = T0 + 60;
        finish(&s, DigiviceEncounterTrail, DigiviceBattleResultLose, false, 0);
        CHECK(s.losses == 1 && s.corruption == DIGIVICE_CORRUPTION_LOSE_GAIN && s.mood == 2);
        CHECK(s.injured_until == T0 + DIGIVICE_INJURY_S && s.enc_foe == DIGIVICE_ENC_NONE);
        if(a >= 4) {
            CHECK(s.distance == digivice_rules_area_length(a) && s.mid_state == 0);
        } else {
            CHECK(s.distance == 100 && s.mid_state == 2);
        }
    }
    // A boss loss in areas 1 to 4 keeps the boss waiting.
    DigiviceState b = with_form(DigiviceFormTagdrake);
    b.distance = 0;
    finish(&b, DigiviceEncounterBoss, DigiviceBattleResultLose, false, 0);
    CHECK(b.distance == 0 && digivice_expedition_boss_ready(&b));
    // A Baby loses mood and gets hurt, but is not corrupted.
    DigiviceState baby = with_form(DigiviceFormBitling);
    baby.mood = 3;
    baby.enc_foe = 0;
    baby.enc_expires = T0 + 60;
    finish(&baby, DigiviceEncounterTrail, DigiviceBattleResultLose, false, 0);
    CHECK(baby.corruption == 0 && baby.mood == 2 && baby.losses == 1);
    CHECK(baby.injured_until == T0 + DIGIVICE_INJURY_S);
    // A loss can push corruption to 100 and transform the pet.
    DigiviceState c = with_form(DigiviceFormBytemon);
    c.corruption = 90;
    finish(&c, DigiviceEncounterTrail, DigiviceBattleResultLose, false, 0);
    CHECK(c.form == DigiviceFormVirubyte && c.prev_form == DigiviceFormBytemon);
    // The injury heals on the tick after it ends.
    c.form = DigiviceFormBytemon;
    digivice_rules_tick(&c, T0 + DIGIVICE_INJURY_S);
    CHECK(c.injured_until == 0);
}

static void test_virux_rewards(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.virux_type = DigiviceTypeUsb;
    s.virux_expires = T0 + 60;
    DigiviceReward r = finish(&s, DigiviceEncounterVirux, DigiviceBattleResultWin, false, 99);
    CHECK(r.exp == 2 * DIGIVICE_WIN_BASE_EXP && s.exp == r.exp);
    CHECK(s.data[DigiviceTypeUsb] == DIGIVICE_WIN_DATA && s.wins == 1);
    CHECK(s.virux_type == DigiviceTypeNone);
    // A VIRUX loss never resets an Expedition streak.
    s.area = 5;
    s.distance = 10;
    s.virux_type = DigiviceTypeNfc;
    finish(&s, DigiviceEncounterVirux, DigiviceBattleResultLose, false, 0);
    CHECK(s.distance == 10 && s.virux_type == DigiviceTypeNone && s.losses == 1);
}

static void test_win_without_virux_type_grants_exp_only(void) {
    // Guard bytes after the state catch a write through data[DigiviceTypeNone].
    struct {
        DigiviceState s;
        uint8_t guard[1024];
    } box;
    box.s = with_form(DigiviceFormBytemon);
    memset(box.guard, 0xAA, sizeof(box.guard));
    finish(&box.s, DigiviceEncounterVirux, DigiviceBattleResultWin, false, 99);
    CHECK(box.s.exp == 2 * DIGIVICE_WIN_BASE_EXP && box.s.wins == 1);
    for(uint8_t t = 0; t < DigiviceTypeCount; t++)
        CHECK(box.s.data[t] == 0);
    for(size_t i = 0; i < sizeof(box.guard); i++) {
        if(box.guard[i] != 0xAA) {
            CHECK(box.guard[i] == 0xAA);
            break;
        }
    }
}

static void test_draw_and_flee_change_nothing_else(void) {
    for(int result = DigiviceBattleResultFled; result <= DigiviceBattleResultDraw; result++) {
        DigiviceState s = with_form(DigiviceFormTagdrake);
        s.area = 6;
        s.distance = 50;
        s.enc_foe = 1;
        s.enc_expires = T0 + 60;
        DigiviceState before = s;
        DigiviceBattleResult res = (DigiviceBattleResult)result;
        DigiviceReward r = finish(&s, DigiviceEncounterTrail, res, false, 0);
        CHECK(r.exp == 0 && r.item == DigiviceItemNone);
        before.enc_foe = DIGIVICE_ENC_NONE;
        before.enc_expires = 0;
        CHECK(memcmp(&s, &before, sizeof(s)) == 0);
    }
    // A fled boss stays at distance 0.
    DigiviceState b = with_form(DigiviceFormTagdrake);
    b.distance = 0;
    finish(&b, DigiviceEncounterBoss, DigiviceBattleResultFled, false, 0);
    CHECK(b.distance == 0 && b.cleared == 0);
}

static void test_stage_tables_cover_every_stage(void) {
    const uint16_t power[DigiviceStageCount] = DIGIVICE_STAGE_POWER;
    const uint8_t shot[DigiviceStageCount] = DIGIVICE_STAGE_SHOT;
    const uint16_t virux[DigiviceStageCount] = DIGIVICE_VIRUX_POWER;
    CHECK(DigiviceStageCount == 7);
    for(int i = 2; i < DigiviceStageCount; i++) {
        CHECK(power[i] > power[i - 1]);
        CHECK(shot[i] >= shot[i - 1] || shot[i] == DIGIVICE_SHOT_BEAM);
        CHECK(virux[i] >= virux[i - 1]);
    }
    CHECK(shot[DigiviceStageMega] == DIGIVICE_SHOT_BEAM && shot[DigiviceStageUltimate] == 7);
}

static void test_battle_result_counts_win_and_lose_but_not_fled_or_draw(void) {
    DigiviceState s = with_form(DigiviceFormTagdrake);
    digivice_rules_battle_result(&s, DigiviceEncounterTrail, DigiviceBattleResultWin, false, 0, 99, T0);
    digivice_rules_battle_result(&s, DigiviceEncounterTrail, DigiviceBattleResultLose, false, 0, 99, T0);
    digivice_rules_battle_result(&s, DigiviceEncounterTrail, DigiviceBattleResultFled, false, 0, 99, T0);
    digivice_rules_battle_result(&s, DigiviceEncounterTrail, DigiviceBattleResultDraw, false, 0, 99, T0);
    CHECK(s.battles == 2 && s.wins_stage == 1);
}

#if DIGIVICE_EASTER_EGGS
static void test_partner_battle_init_and_result(void) {
    DigiviceState s = with_form(DigiviceFormKorno);
    s.mood = 4;
    s.enc_foe = DIGIVICE_ENC_HOMAGE + 2;
    s.enc_expires = T0 + 1000;
    DigiviceBattle b;
    digivice_battle_init(&b, &s, DigiviceEncounterTrail, T0);
    CHECK(b.foe_class == DigiviceFoeClassHomage && b.foe == 2);
    CHECK(b.foe_power == 20 * DIGIVICE_HOMAGE_X10 / 10);
    CHECK(b.foe_attr == digivice_homage_attr(2));
    CHECK(b.foe_shot == b.player_shot);

    s.special_count = 3;
    DigiviceReward r = digivice_rules_battle_result(
        &s, DigiviceEncounterTrail, DigiviceBattleResultWin, false, T0 + 1000, 50, T0);
    CHECK(r.capture == DigiviceCaptureCaught || r.capture == DigiviceCaptureEscaped);
    CHECK(r.partner == 2);
    CHECK(r.exp == (DIGIVICE_TRAIL_EXP_BASE + DIGIVICE_TRAIL_EXP_AREA) * DIGIVICE_SPECIAL_EXP_X);
    CHECK(s.enc_foe == DIGIVICE_ENC_NONE);
    // The partner fight itself does not count towards the next one.
    CHECK(r.capture == DigiviceCaptureEscaped ? s.special_count == DIGIVICE_HOMAGE_RETRY :
                                                 s.special_count == 3);
}

static void test_partner_loss_and_counting(void) {
    DigiviceState s = with_form(DigiviceFormKorno);
    s.enc_foe = DIGIVICE_ENC_HOMAGE;
    s.enc_expires = T0 + 1000;
    digivice_rules_battle_result(
        &s, DigiviceEncounterTrail, DigiviceBattleResultLose, false, T0 + 1000, 50, T0);
    CHECK(s.special_count == DIGIVICE_HOMAGE_RETRY);
    CHECK(s.caught == 0);

    DigiviceState t = with_form(DigiviceFormKorno);
    t.enc_foe = 0;
    t.enc_expires = T0 + 1000;
    digivice_rules_battle_result(
        &t, DigiviceEncounterTrail, DigiviceBattleResultWin, false, T0 + 1000, 50, T0);
    CHECK(t.special_count == 1);
    digivice_rules_battle_result(
        &t, DigiviceEncounterTrail, DigiviceBattleResultFled, false, 0, 50, T0);
    CHECK(t.special_count == 1);
}
#endif

int main(void) {
    test_stage_tables_cover_every_stage();
    test_hit_pct();
    test_power();
    test_charge();
    test_guard();
    test_shot_outcomes();
    test_foe_turn_symmetry();
    test_three_hits();
    test_round_cap();
    test_shield_cancels_first_hit();
    test_battle_init();
    test_can_battle();
    test_retreat();
    test_stale_fight_keeps_newer_encounter();
    test_trail_rewards();
    test_battle_result_counts_win_and_lose_but_not_fled_or_draw();
    test_boss_rewards();
    test_defeat_penalties();
    test_virux_rewards();
    test_win_without_virux_type_grants_exp_only();
    test_draw_and_flee_change_nothing_else();
#if DIGIVICE_EASTER_EGGS
    test_partner_battle_init_and_result();
    test_partner_loss_and_counting();
#endif

    if(failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("digivice_battle: all checks passed\n");
    return EXIT_SUCCESS;
}
