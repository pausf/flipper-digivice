// Host-side test for the Expedition rules. Run: applications/services/digivice/test/run.sh
#include "../digivice_expedition.h"
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

// A seed whose first xorshift32 draw is `pct` modulo 100.
static uint32_t seed_with_pct(uint8_t pct) {
    for(uint32_t seed = 1;; seed++) {
        DigiviceState probe;
        memset(&probe, 0, sizeof(probe));
        probe.rng = seed;
        if(digivice_rules_rng(&probe) % 100 == pct) return seed;
    }
}

static void test_tables(void) {
    static const uint16_t trail_q[DIGIVICE_AREA_COUNT][2] = {
        {10, 10}, {16, 16}, {24, 24}, {34, 34}, {59, 46}, {78, 60}, {98, 76}, {122, 122}};
    static const uint16_t boss_q[DIGIVICE_AREA_COUNT] = {14, 22, 33, 47, 64, 84, 106, 131};
    for(uint8_t a = 0; a < DIGIVICE_AREA_COUNT; a++) {
        for(uint8_t i = 0; i < 2; i++) {
            DigiviceFoe foe = digivice_expedition_pool_foe(a, i);
            CHECK(foe < DigiviceFoeCount);
            CHECK(digivice_expedition_foe_power(a, foe) == trail_q[a][i]);
        }
        CHECK(digivice_expedition_boss_power(a) == boss_q[a]);
        CHECK(digivice_expedition_streak_area(a) == (a >= 4));
    }
    CHECK(digivice_expedition_pool_foe(0, 0) == DigiviceFoeEstatik);
    CHECK(digivice_expedition_pool_foe(7, 1) == DigiviceFoeFuzzcan);
    CHECK(digivice_expedition_foe_attr(DigiviceFoeKorno) == DigiviceAttrPortador);
    CHECK(digivice_expedition_foe_attr(DigiviceFoeChipko) == DigiviceAttrCifrado);
    CHECK(digivice_expedition_foe_attr(DigiviceFoeChatarro) == DigiviceAttrNulo);
    CHECK(!digivice_expedition_foe_champion(DigiviceFoeMokobit));
    CHECK(digivice_expedition_foe_champion(DigiviceFoeFuzzcan));
    CHECK(digivice_expedition_boss_attr(2) == DigiviceAttrPortador);
    CHECK(digivice_expedition_boss_attr(7) == DigiviceAttrNulo);
    CHECK(strcmp(digivice_expedition_boss_name(0), "CHISPAPO") == 0);
    CHECK(strcmp(digivice_expedition_foe_name(DigiviceFoeVampixel), "VAMPIXEL") == 0);
    CHECK(digivice_expedition_midway_kind(0) == DigiviceMidwaySpring);
    CHECK(digivice_expedition_midway_kind(1) == DigiviceMidwayCache);
    CHECK(digivice_expedition_midway_kind(2) == DigiviceMidwayShortcut);
    CHECK(digivice_expedition_midway_kind(7) == DigiviceMidwayCache);
}

static void test_one_pace_per_awake_minute(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    digivice_rules_tick(&s, T0 + 59);
    CHECK(s.distance == 480 && s.pace_acc_s == 59);
    digivice_rules_tick(&s, T0 + 60);
    CHECK(s.distance == 479 && s.pace_acc_s == 0);
    // 30 s + 30 s carry into one pace.
    digivice_rules_tick(&s, T0 + 90);
    CHECK(s.distance == 479 && s.pace_acc_s == 30);
    digivice_rules_tick(&s, T0 + 120);
    CHECK(s.distance == 478 && s.paces_since_fight <= 2);
}

static void test_baby_walks_egg_and_sick_do_not(void) {
    DigiviceState baby = with_form(DigiviceFormBitling);
    digivice_rules_tick(&baby, T0 + 600);
    CHECK(baby.distance == 470);

    DigiviceState egg = with_form(DigiviceFormByteegg);
    digivice_rules_tick(&egg, T0 + 600);
    CHECK(egg.distance == 480 && egg.pace_acc_s == 0);

    DigiviceState sick = with_form(DigiviceFormBytemon);
    sick.sick = 1;
    sick.doses_needed = 1;
    digivice_rules_tick(&sick, T0 + 600);
    CHECK(sick.distance == 480);

    DigiviceState boss = with_form(DigiviceFormBytemon);
    boss.distance = 0;
    uint32_t rng = boss.rng;
    digivice_rules_tick(&boss, T0 + 600);
    CHECK(boss.distance == 0 && boss.rng == rng && boss.enc_foe == DIGIVICE_ENC_NONE);
}

static void test_no_paces_asleep(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.distance = 1000;
    uint32_t night = DAY(10) + H(23);
    s.last_tick = night;
    digivice_rules_tick(&s, night + H(8));
    CHECK(s.distance == 1000);
    digivice_rules_tick(&s, night + H(8) + 600);
    CHECK(s.distance == 990);
}

static void test_one_hour_tick_matches_sixty_minute_ticks(void) {
    DigiviceState a = with_form(DigiviceFormTagdrake);
    DigiviceState b = a;
    for(uint32_t t = T0 + 60; t <= T0 + H(10); t += 60)
        digivice_rules_tick(&a, t);
    digivice_rules_tick(&b, T0 + H(10));
    CHECK(a.distance == b.distance && a.distance < 480);
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
}

static void test_48h_catch_up_is_clamped(void) {
    DigiviceState a = with_form(DigiviceFormTagdrake);
    a.area = 7;
    a.distance = 1440;
    DigiviceState b = a;
    a.last_tick = T0 - DAY(5);
    b.last_tick = T0;
    digivice_rules_tick(&a, T0 + H(48));
    digivice_rules_tick(&b, T0 + H(48));
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
    CHECK(a.distance < 1440);

    // Without care rules in the way, the 32 awake hours of a 48 h window are 1,920 paces.
    DigiviceState c = with_form(DigiviceFormTagdrake);
    c.distance = 5000;
    digivice_expedition_walk(&c, H(32), T0 + H(32));
    CHECK(c.distance == 5000 - 1920);
}

static void test_exact_pace_instant(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.pace_acc_s = 30;
    s.paces_since_fight = DIGIVICE_ENCOUNTER_GUARANTEE - 1;
    digivice_rules_tick(&s, T0 + 90);
    // The pace fell at T0 + 30, so that is when the guaranteed encounter spawned.
    CHECK(s.enc_foe <= 1);
    CHECK(s.enc_expires == T0 + 30 + DIGIVICE_ENCOUNTER_TTL_S);
    CHECK(s.paces_since_fight == 1);
}

static void test_encounter_roll(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.rng = seed_with_pct(DIGIVICE_ENCOUNTER_PCT - 1);
    s.paces_since_fight = 10;
    digivice_expedition_walk(&s, 60, T0 + 60);
    CHECK(s.enc_foe <= 1 && s.paces_since_fight == 0);
    CHECK(s.enc_expires == T0 + 60 + DIGIVICE_ENCOUNTER_TTL_S);

    DigiviceState t = with_form(DigiviceFormBytemon);
    t.rng = seed_with_pct(DIGIVICE_ENCOUNTER_PCT);
    t.paces_since_fight = 10;
    digivice_expedition_walk(&t, 60, T0 + 60);
    CHECK(t.enc_foe == DIGIVICE_ENC_NONE && t.paces_since_fight == 11);
}

static void test_guarantee_at_pace_50(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.rng = seed_with_pct(99);
    s.paces_since_fight = DIGIVICE_ENCOUNTER_GUARANTEE - 2;
    digivice_expedition_walk(&s, 60, T0 + 60);
    CHECK(s.enc_foe == DIGIVICE_ENC_NONE);
    s.rng = seed_with_pct(99);
    digivice_expedition_walk(&s, 60, T0 + 120);
    CHECK(s.enc_foe <= 1 && s.paces_since_fight == 0);
}

static void test_no_roll_while_pending(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.enc_foe = 1;
    s.enc_expires = T0 + H(1);
    s.paces_since_fight = DIGIVICE_ENCOUNTER_GUARANTEE;
    uint32_t rng = s.rng;
    digivice_expedition_walk(&s, 600, T0 + 600);
    CHECK(s.enc_foe == 1 && s.enc_expires == T0 + H(1) && s.rng == rng);
    CHECK(s.distance == 470 && s.paces_since_fight == DIGIVICE_ENCOUNTER_GUARANTEE);

    DigiviceState v = with_form(DigiviceFormBytemon);
    v.virux_type = DigiviceTypeIr;
    v.virux_expires = T0 + H(1);
    v.paces_since_fight = DIGIVICE_ENCOUNTER_GUARANTEE;
    digivice_expedition_walk(&v, 600, T0 + 600);
    CHECK(v.enc_foe == DIGIVICE_ENC_NONE);
}

static void test_pool_index_range(void) {
    bool seen[2] = {false, false};
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.area = 7;
    s.distance = 1440;
    for(int i = 0; i < 200; i++) {
        s.enc_foe = DIGIVICE_ENC_NONE;
        s.paces_since_fight = DIGIVICE_ENCOUNTER_GUARANTEE;
        digivice_expedition_walk(&s, 60, T0 + 60);
        CHECK(s.enc_foe <= 1);
        if(s.enc_foe <= 1) seen[s.enc_foe] = true;
    }
    CHECK(seen[0] && seen[1]);
}

static void test_encounter_expires_without_penalty(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.enc_foe = 0;
    s.enc_expires = T0 + DIGIVICE_ENCOUNTER_TTL_S;
    uint16_t losses = s.losses;
    uint8_t mood = s.mood;
    digivice_rules_tick(&s, T0 + DIGIVICE_ENCOUNTER_TTL_S - 1);
    CHECK(digivice_expedition_trail_pending(&s, T0 + DIGIVICE_ENCOUNTER_TTL_S - 1));
    digivice_rules_tick(&s, T0 + DIGIVICE_ENCOUNTER_TTL_S);
    CHECK(s.enc_foe == DIGIVICE_ENC_NONE && s.enc_expires == 0);
    CHECK(s.losses == losses && s.mood == mood && s.corruption == 0);
}

static void test_offline_catch_up_leaves_no_stale_encounter(void) {
    for(uint32_t seed = 1; seed < 40; seed++) {
        DigiviceState s = with_form(DigiviceFormTagdrake);
        s.area = 7;
        s.distance = 1440;
        s.rng = seed * 2654435761UL;
        digivice_rules_tick(&s, T0 + H(14));
        CHECK(s.enc_foe == DIGIVICE_ENC_NONE || s.enc_expires > T0 + H(14));
    }
}

static void test_virux_and_trail_never_coexist(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.mode = DigiviceModeHacker;
    s.enc_foe = 0;
    s.enc_expires = T0 + 600;
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedNfcRead, 1, T0, 0));
    CHECK(s.virux_type == DigiviceTypeNone);
    s.enc_foe = DIGIVICE_ENC_NONE;
    s.enc_expires = 0;
    CHECK(digivice_rules_on_deed(&s, DolphinDeedNfcRead, 1, T0, 0));
    CHECK(s.virux_type == DigiviceTypeNfc);
}

static void test_hike_judge(void) {
    uint8_t beat = 0xFF;
    CHECK(digivice_expedition_hike_phase(500, &beat) == 0 && beat == 0);
    CHECK(digivice_expedition_hike_phase(740, &beat) == 240 && beat == 0);
    CHECK(digivice_expedition_hike_phase(760, &beat) == -240 && beat == 1);
    CHECK(digivice_expedition_hike_phase(0, &beat) == -500 && beat == 0);
    CHECK(digivice_expedition_hike_phase(13000, &beat) == 1000 && beat == 23);
    CHECK(digivice_expedition_hike_tap_good(149) && digivice_expedition_hike_tap_good(-149));
    CHECK(digivice_expedition_hike_tap_good(150) && digivice_expedition_hike_tap_good(-150));
    CHECK(!digivice_expedition_hike_tap_good(151) && !digivice_expedition_hike_tap_good(-151));
    CHECK(digivice_expedition_hike_tap_good(0));
}

static void test_hike_cap_and_window(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.area = 7;
    s.distance = 1440;
    CHECK(digivice_expedition_hike_left(&s, T0) == DIGIVICE_HIKE_CAP);
    CHECK(digivice_expedition_hike(&s, 24, T0) == 24);
    CHECK(s.bonus_window_start == T0 && s.distance == 1416);
    CHECK(digivice_expedition_hike(&s, 24, T0 + 60) == 24);
    CHECK(digivice_expedition_hike(&s, 24, T0 + 120) == 12);
    CHECK(digivice_expedition_hike_left(&s, T0 + 120) == 0);
    CHECK(digivice_expedition_hike(&s, 24, T0 + 3599) == 0);
    CHECK(s.distance == 1440 - 60);
    // The window rolls over exactly one hour after it opened.
    CHECK(digivice_expedition_hike_left(&s, T0 + 3600) == DIGIVICE_HIKE_CAP);
    CHECK(digivice_expedition_hike(&s, 200, T0 + 3600) == DIGIVICE_HIKE_CAP);
    CHECK(s.bonus_window_start == T0 + 3600);

    // Asleep, sick or an Egg earns nothing.
    DigiviceState night = with_form(DigiviceFormBytemon);
    CHECK(digivice_expedition_hike(&night, 10, DAY(10) + H(23) + 60) == 0);
    DigiviceState sick = with_form(DigiviceFormBytemon);
    sick.sick = 1;
    CHECK(digivice_expedition_hike(&sick, 10, T0) == 0);
    DigiviceState egg = with_form(DigiviceFormByteegg);
    CHECK(digivice_expedition_hike(&egg, 10, T0) == 0);

    // The last paces of an area stop at the boss.
    DigiviceState end = with_form(DigiviceFormBytemon);
    end.distance = 5;
    CHECK(digivice_expedition_hike(&end, 24, T0) == 5);
    CHECK(end.distance == 0 && end.hike_count == 5);
}

static void test_scan_paces(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.mode = DigiviceModeHacker;
    s.area = 7;
    s.distance = 1440;
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedNfcRead, 1, T0, 99));
    CHECK(s.distance == 1440 - DIGIVICE_SCAN_PACES && s.scan_count == DIGIVICE_SCAN_PACES);
    // A deed that does not scan grants no paces.
    digivice_rules_on_deed(&s, DolphinDeedIrSend, 1, T0, 99);
    CHECK(s.distance == 1440 - DIGIVICE_SCAN_PACES);
    for(int i = 0; i < 20; i++)
        digivice_rules_on_deed(&s, DolphinDeedSubGhzReceiverInfo, 1, T0 + 10, 99);
    CHECK(s.scan_count == DIGIVICE_SCAN_CAP && s.distance == 1440 - DIGIVICE_SCAN_CAP);

    // Scans and Hike share the window: the scan opened it, the Hike keeps it.
    CHECK(digivice_expedition_hike(&s, 10, T0 + 20) == 10);
    CHECK(s.bonus_window_start == T0);

    DigiviceState normal = with_form(DigiviceFormBytemon);
    CHECK(!digivice_rules_on_deed(&normal, DolphinDeedNfcRead, 1, T0, 99));
    CHECK(normal.distance == 480 && normal.scan_count == 0);
}

static void test_midway(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.distance = 241;
    s.paces_since_fight = 0;
    s.rng = seed_with_pct(99);
    digivice_expedition_walk(&s, 60, T0 + 60);
    CHECK(s.distance == 240 && s.mid_state == 1);
    CHECK(digivice_rules_attention(&s, T0 + 60));

    // Spring: area 1.
    s.hunger = 1;
    s.mood = 0;
    s.corruption = 30;
    s.injured_until = T0 + 600;
    s.starving_since = 0;
    DigiviceMidwayResult r = digivice_expedition_midway_claim(&s, true, 0, T0 + 60);
    CHECK(r.kind == DigiviceMidwaySpring && r.ok);
    CHECK(s.hunger == DIGIVICE_HUNGER_MAX && s.mood == DIGIVICE_MOOD_MAX);
    CHECK(s.corruption == 20 && s.injured_until == 0 && s.mid_state == 2);
    // Claimed once: it never comes back in this area.
    r = digivice_expedition_midway_claim(&s, true, 0, T0 + 60);
    CHECK(r.kind == DigiviceMidwayCount && !r.ok);
    s.distance = 100;
    digivice_expedition_walk(&s, 60, T0 + 120);
    CHECK(s.mid_state == 2);

    // Cache: area 2, a Shield, then EXP once one is held.
    DigiviceState c = with_form(DigiviceFormBytemon);
    c.area = 1;
    c.mid_state = 1;
    r = digivice_expedition_midway_claim(&c, true, 0, T0);
    CHECK(r.kind == DigiviceMidwayCache && r.ok && c.item == DigiviceItemShield);
    c.mid_state = 1;
    uint32_t exp = c.exp;
    r = digivice_expedition_midway_claim(&c, true, 0, T0);
    CHECK(!r.ok && c.exp == exp + DIGIVICE_CACHE_EXP && c.item == DigiviceItemShield);

    // Shortcut: area 3, length 720, a jump of 144.
    DigiviceState k = with_form(DigiviceFormBytemon);
    k.area = 2;
    k.mid_state = 1;
    k.distance = 360;
    r = digivice_expedition_midway_claim(&k, false, 0, T0);
    CHECK(r.kind == DigiviceMidwayShortcut && !r.ok && k.distance == 360 && k.mid_state == 2);
    k.mid_state = 1;
    r = digivice_expedition_midway_claim(&k, true, DIGIVICE_SHORTCUT_WIN_PCT - 1, T0);
    CHECK(r.ok && k.distance == 216);
    k.mid_state = 1;
    r = digivice_expedition_midway_claim(&k, true, DIGIVICE_SHORTCUT_WIN_PCT, T0);
    CHECK(!r.ok && k.distance == 360);
    k.mid_state = 1;
    k.distance = 700;
    digivice_expedition_midway_claim(&k, true, 99, T0);
    CHECK(k.distance == 720);
    k.mid_state = 1;
    k.distance = 100;
    digivice_expedition_midway_claim(&k, true, 0, T0);
    CHECK(k.distance == 0 && digivice_expedition_boss_ready(&k));
}

static void test_boss_gate_and_clear(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.distance = 1;
    s.mid_state = 2;
    digivice_expedition_walk(&s, 60, T0 + 60);
    CHECK(s.distance == 0 && digivice_expedition_boss_ready(&s));
    CHECK(!digivice_expedition_boss_gate_open(&s));
    CHECK(!digivice_rules_attention(&s, T0 + 60));
    s.form = DigiviceFormVirubyte;
    s.prev_form = DigiviceFormBytemon;
    CHECK(!digivice_expedition_boss_gate_open(&s));
    s.prev_form = DigiviceFormWavedrake;
    CHECK(digivice_expedition_boss_gate_open(&s));
    s.form = DigiviceFormWavedrake;
    CHECK(digivice_expedition_boss_gate_open(&s));
    CHECK(digivice_rules_attention(&s, T0 + 60));

    digivice_expedition_boss_won(&s);
    CHECK(s.cleared == 0x01 && s.area == 1 && s.distance == 600 && s.mid_state == 0);
    CHECK(digivice_expedition_area_unlocked(&s, 1) && !digivice_expedition_area_unlocked(&s, 2));

    // A repeat clear of area 1 moves on to the lowest uncleared area, 2.
    CHECK(digivice_expedition_select_area(&s, 0));
    s.distance = 0;
    digivice_expedition_boss_won(&s);
    CHECK(s.cleared == 0x01 && s.area == 1);

    s.cleared = 0x7F;
    s.area = 7;
    s.distance = 0;
    digivice_expedition_boss_won(&s);
    CHECK(s.cleared == 0xFF && s.area == 7 && s.distance == 0);
    CHECK(digivice_expedition_all_clear(&s) && !digivice_expedition_boss_ready(&s));
    CHECK(!digivice_rules_attention(&s, T0));
}

static void test_select_area(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    s.distance = 100;
    s.mid_state = 2;
    CHECK(!digivice_expedition_select_area(&s, 1));
    CHECK(!digivice_expedition_select_area(&s, 8));
    CHECK(s.area == 0 && s.distance == 100);
    // The current area changes nothing.
    CHECK(digivice_expedition_select_area(&s, 0));
    CHECK(s.distance == 100 && s.mid_state == 2);
    s.cleared = 0x03;
    s.enc_foe = 1;
    s.enc_expires = T0 + 60;
    s.paces_since_fight = 20;
    CHECK(digivice_expedition_select_area(&s, 2));
    CHECK(s.area == 2 && s.distance == 720 && s.mid_state == 0);
    CHECK(s.enc_foe == DIGIVICE_ENC_NONE && s.paces_since_fight == 0);
}

static void test_streak_lost(void) {
    DigiviceState s = with_form(DigiviceFormTagdrake);
    s.area = 3;
    s.distance = 0;
    s.mid_state = 2;
    digivice_expedition_streak_lost(&s);
    CHECK(s.distance == 0 && s.mid_state == 2);
    s.area = 4;
    s.paces_since_fight = 9;
    digivice_expedition_streak_lost(&s);
    CHECK(s.distance == 960 && s.mid_state == 0 && s.paces_since_fight == 0);
}

static void test_attention(void) {
    DigiviceState s = with_form(DigiviceFormBytemon);
    CHECK(!digivice_rules_attention(&s, T0));
    s.enc_foe = 0;
    s.enc_expires = T0 + 10;
    CHECK(digivice_rules_attention(&s, T0));
    CHECK(!digivice_rules_attention(&s, T0 + 10));
    s.enc_foe = DIGIVICE_ENC_NONE;
    s.virux_type = DigiviceTypeUsb;
    s.virux_expires = T0 + 10;
    CHECK(digivice_rules_attention(&s, T0));
}

int main(void) {
    test_tables();
    test_one_pace_per_awake_minute();
    test_baby_walks_egg_and_sick_do_not();
    test_no_paces_asleep();
    test_one_hour_tick_matches_sixty_minute_ticks();
    test_48h_catch_up_is_clamped();
    test_exact_pace_instant();
    test_encounter_roll();
    test_guarantee_at_pace_50();
    test_no_roll_while_pending();
    test_pool_index_range();
    test_encounter_expires_without_penalty();
    test_offline_catch_up_leaves_no_stale_encounter();
    test_virux_and_trail_never_coexist();
    test_hike_judge();
    test_hike_cap_and_window();
    test_scan_paces();
    test_midway();
    test_boss_gate_and_clear();
    test_select_area();
    test_streak_lost();
    test_attention();

    if(failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("digivice_expedition: all checks passed\n");
    return EXIT_SUCCESS;
}
