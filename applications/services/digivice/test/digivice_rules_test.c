// Host-side test for the pure digivice rules. Run: applications/services/digivice/test/run.sh
#include "../digivice_rules.h"
#include "../digivice_config.h"

#include <stddef.h>
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

static DigiviceState fresh(uint32_t now) {
    DigiviceState s;
    digivice_rules_init(&s, now);
    return s;
}

static DigiviceState rookie(uint32_t now) {
    DigiviceState s = fresh(now);
    s.form = DigiviceFormBytemon;
    s.exp = DIGIVICE_ROOKIE_EXP;
    return s;
}

// Deeds only count in Hacker mode; the v1 deed tests run there.
static DigiviceState hacker(DigiviceState s) {
    s.mode = DigiviceModeHacker;
    return s;
}

static void test_init(void) {
    DigiviceState s = fresh(T0);
    CHECK(s.form == DigiviceFormByteegg);
    CHECK(s.hunger == DIGIVICE_HUNGER_MAX);
    CHECK(s.mood == DIGIVICE_MOOD_MAX);
    CHECK(s.corruption == 0);
    CHECK(s.virux_type == DigiviceTypeNone);
    CHECK(s.evo_from == DigiviceFormNone);
    CHECK(s.last_tick == T0 && s.born_at == T0);
}

static void test_hunger_decays_every_2h(void) {
    DigiviceState s = fresh(T0);
    digivice_rules_tick(&s, T0 + H(2) - 1);
    CHECK(s.hunger == 4);
    digivice_rules_tick(&s, T0 + H(2));
    CHECK(s.hunger == 3);
    digivice_rules_tick(&s, T0 + H(4));
    CHECK(s.hunger == 2);
}

static void test_mood_decays_every_3h(void) {
    DigiviceState s = fresh(T0);
    digivice_rules_tick(&s, T0 + H(3) - 1);
    CHECK(s.mood == 4);
    digivice_rules_tick(&s, T0 + H(3));
    CHECK(s.mood == 3);
}

static void test_60s_ticks_lose_nothing(void) {
    DigiviceState s = fresh(T0);
    for(uint32_t t = T0 + 60; t <= T0 + H(6); t += 60)
        digivice_rules_tick(&s, t);
    CHECK(s.hunger == 1);
    CHECK(s.mood == 2);
}

static void test_sleep_window(void) {
    CHECK(digivice_rules_is_sleep_hour(23));
    CHECK(digivice_rules_is_sleep_hour(0));
    CHECK(digivice_rules_is_sleep_hour(6));
    CHECK(!digivice_rules_is_sleep_hour(7));
    CHECK(!digivice_rules_is_sleep_hour(22));

    // 22:00 -> 08:00 has one awake hour before 23:00 and one after 07:00.
    CHECK(digivice_rules_awake_seconds(DAY(10) + H(22), DAY(11) + H(8)) == H(2));

    DigiviceState s = fresh(DAY(10) + H(23));
    digivice_rules_tick(&s, DAY(11) + H(7));
    CHECK(s.hunger == 4);
    CHECK(s.mood == 4);
}

static void test_elapsed_clamped_to_48h(void) {
    // With hunger at 0, corruption accrues on every awake second and its sub-hour remainder
    // survives saturation. Any 48 h window holds exactly 32 awake hours (remainder 0), while
    // an unclamped 30-day gap ends 1234 s into an awake hour (remainder 1234).
    DigiviceState a = fresh(T0);
    DigiviceState b = fresh(T0);
    a.hunger = 0;
    b.hunger = 0;
    digivice_rules_tick(&a, T0 + H(48));
    digivice_rules_tick(&b, T0 + DAY(30) + 1234);
    CHECK(a.corrupt_acc_s == 0);
    CHECK(b.corrupt_acc_s == a.corrupt_acc_s);
    CHECK(b.last_tick == T0 + DAY(30) + 1234);
}

static void test_clock_backwards_is_zero_elapsed(void) {
    DigiviceState s = fresh(T0);
    digivice_rules_tick(&s, T0 - H(5));
    CHECK(s.hunger == 4 && s.mood == 4);
    CHECK(s.last_tick == T0 - H(5));
}

static void test_corruption_grows_5_per_hour_at_zero(void) {
    DigiviceState s = rookie(T0);
    s.hunger = 0;
    digivice_rules_tick(&s, T0 + H(1) - 60);
    CHECK(s.corruption == 0);
    digivice_rules_tick(&s, T0 + H(1));
    CHECK(s.corruption == 5);
    digivice_rules_tick(&s, T0 + H(3));
    CHECK(s.corruption == 15);

    DigiviceState m = rookie(T0);
    m.mood = 0;
    digivice_rules_tick(&m, T0 + H(2));
    CHECK(m.corruption == 10);
}

static void test_corruption_starts_when_hunger_hits_zero(void) {
    DigiviceState s = rookie(T0);
    s.hunger = 1;
    s.mood = 4;
    // Hunger reaches 0 at +2h, then 1h more of starvation adds 5.
    digivice_rules_tick(&s, T0 + H(3));
    CHECK(s.hunger == 0);
    CHECK(s.corruption == 5);
}

static void test_feed(void) {
    DigiviceState s = rookie(T0);
    s.hunger = 2;
    s.corruption = 12;
    CHECK(digivice_rules_feed(&s, T0));
    CHECK(s.hunger == 3);
    CHECK(s.corruption == 7);
    s.hunger = DIGIVICE_HUNGER_MAX;
    CHECK(!digivice_rules_feed(&s, T0));
    CHECK(s.corruption == 7);
    s.hunger = 0;
    s.corruption = 3;
    digivice_rules_feed(&s, T0);
    CHECK(s.corruption == 0);
}

static void test_train(void) {
    DigiviceState s = rookie(T0);
    s.hunger = 3;
    s.mood = 2;
    s.corruption = 15;
    digivice_rules_train(&s, true, T0);
    CHECK(s.hunger == 2 && s.mood == 3 && s.strength == 1 && s.corruption == 5);
    digivice_rules_train(&s, false, T0);
    CHECK(s.hunger == 1 && s.mood == 3 && s.strength == 1 && s.corruption == 5);
    s.hunger = 0;
    digivice_rules_train(&s, true, T0);
    CHECK(s.hunger == 0 && s.corruption == 0);
}

static void test_miss_chance_from_50(void) {
    DigiviceState s = rookie(T0);
    s.corruption = 49;
    CHECK(!digivice_rules_glitchy(&s));
    CHECK(!digivice_rules_attack_misses(&s, 0));
    s.corruption = 50;
    CHECK(digivice_rules_glitchy(&s));
    CHECK(digivice_rules_attack_misses(&s, 24));
    CHECK(!digivice_rules_attack_misses(&s, 25));
}

static void test_transform_at_100_saves_form(void) {
    DigiviceState s = rookie(T0);
    s.form = DigiviceFormWavedrake;
    s.hunger = 0;
    s.corruption = 95;
    digivice_rules_tick(&s, T0 + H(1));
    CHECK(s.corruption == 100);
    CHECK(s.form == DigiviceFormVirubyte);
    CHECK(s.prev_form == DigiviceFormWavedrake);
    CHECK(s.evo_from == DigiviceFormWavedrake);
    CHECK(digivice_rules_stage(&s) == DigiviceStageChampion);
}

static void test_baby_never_transforms(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormBitling;
    s.hunger = 0;
    s.corruption = 95;
    digivice_rules_tick(&s, T0 + H(2));
    CHECK(s.corruption == 100);
    CHECK(s.form == DigiviceFormBitling);
}

static void test_recovery_after_24h_clean(void) {
    DigiviceState s = rookie(T0);
    s.form = DigiviceFormVirubyte;
    s.prev_form = DigiviceFormBeamdrake;
    s.corruption = 10;
    s.clean_since = 0;
    digivice_rules_train(&s, true, T0);
    CHECK(s.corruption == 0);
    CHECK(s.clean_since == T0);

    // Pretend the owner kept it fed: only the last minute is ticked.
    s.hunger = 4;
    s.mood = 4;
    s.last_tick = T0 + H(24) - 120;
    digivice_rules_tick(&s, T0 + H(24) - 60);
    CHECK(s.form == DigiviceFormVirubyte);
    digivice_rules_tick(&s, T0 + H(24));
    CHECK(s.form == DigiviceFormBeamdrake);
    CHECK(s.evo_from == DigiviceFormVirubyte);
}

static void test_recovery_resets_when_corruption_returns(void) {
    DigiviceState s = rookie(T0);
    s.form = DigiviceFormVirubyte;
    s.prev_form = DigiviceFormBytemon;
    s.hunger = 2;
    s.corruption = 5;
    s.clean_since = 0;
    CHECK(digivice_rules_feed(&s, T0));
    CHECK(s.clean_since == T0);
    s.corruption = 0;
    s.hunger = 0;
    s.last_tick = T0 + H(12);
    digivice_rules_tick(&s, T0 + H(13));
    CHECK(s.corruption == 5 && s.clean_since == 0);

    // Clean again at T1, 14 h after T0: the 24 h clock must restart there.
    uint32_t t1 = T0 + H(14);
    CHECK(digivice_rules_feed(&s, t1));
    CHECK(s.corruption == 0);
    CHECK(s.clean_since == t1);

    // Owner keeps it fed. At T0 + 24 h the stale clock would have fired, the new one has not.
    s.hunger = DIGIVICE_HUNGER_MAX;
    s.mood = DIGIVICE_MOOD_MAX;
    s.last_tick = T0 + H(24) - 60;
    digivice_rules_tick(&s, T0 + H(24));
    CHECK(s.form == DigiviceFormVirubyte);

    s.last_tick = t1 + H(24) - 120;
    digivice_rules_tick(&s, t1 + H(24) - 60);
    CHECK(s.form == DigiviceFormVirubyte);

    digivice_rules_tick(&s, t1 + H(24));
    CHECK(s.form == DigiviceFormBytemon);
    CHECK(s.evo_from == DigiviceFormVirubyte);
}

static void test_timing_zone(void) {
    CHECK(!digivice_rules_timing_hit(7, 8));
    CHECK(digivice_rules_timing_hit(8, 8));
    CHECK(digivice_rules_timing_hit(8 + DIGIVICE_METER_ZONE - 1, 8));
    CHECK(!digivice_rules_timing_hit(8 + DIGIVICE_METER_ZONE, 8));
}

static void test_same_visible_ignores_bookkeeping(void) {
    DigiviceState a = fresh(T0);
    DigiviceState b = a;
    b.last_tick += 60;
    b.hunger_acc_s += 60;
    b.mood_acc_s += 60;
    b.corrupt_acc_s += 60;
    CHECK(digivice_rules_same_visible(&a, &b));
    b.hunger--;
    CHECK(!digivice_rules_same_visible(&a, &b));
}

static void test_egg_hatches_after_300s(void) {
    DigiviceState s = fresh(T0);
    CHECK(!digivice_rules_evolve(&s, T0 + 299));
    CHECK(s.form == DigiviceFormByteegg);
    CHECK(digivice_rules_evolve(&s, T0 + 300));
    CHECK(s.form == DigiviceFormBitling);
    CHECK(s.evo_from == DigiviceFormByteegg);
    digivice_rules_ack_evolution(&s);
    CHECK(s.evo_from == DigiviceFormNone);
}

static void test_baby_to_rookie_at_50_exp(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormBitling;
    s.exp = 49;
    CHECK(!digivice_rules_evolve(&s, T0));
    s.exp = 50;
    CHECK(digivice_rules_evolve(&s, T0));
    CHECK(s.form == DigiviceFormBytemon);
}

static DigiviceForm champion_for(uint16_t nfc, uint16_t subghz, uint16_t ir, uint16_t usb) {
    DigiviceState s = rookie(T0);
    s.exp = DIGIVICE_CHAMPION_EXP;
    s.data[DigiviceTypeNfc] = nfc;
    s.data[DigiviceTypeSubGhz] = subghz;
    s.data[DigiviceTypeIr] = ir;
    s.data[DigiviceTypeUsb] = usb;
    digivice_rules_evolve(&s, T0);
    return (DigiviceForm)s.form;
}

static void test_champion_paths_and_ties(void) {
    DigiviceState s = rookie(T0);
    s.exp = DIGIVICE_CHAMPION_EXP - 1;
    CHECK(!digivice_rules_evolve(&s, T0));

    CHECK(champion_for(9, 1, 1, 1) == DigiviceFormTagdrake);
    CHECK(champion_for(1, 9, 1, 1) == DigiviceFormWavedrake);
    CHECK(champion_for(1, 1, 9, 1) == DigiviceFormBeamdrake);
    CHECK(champion_for(1, 1, 1, 9) == DigiviceFormPlugdrake);
    CHECK(champion_for(5, 5, 5, 5) == DigiviceFormTagdrake);
    CHECK(champion_for(1, 5, 5, 5) == DigiviceFormWavedrake);
    CHECK(champion_for(1, 1, 5, 5) == DigiviceFormBeamdrake);
    CHECK(champion_for(0, 0, 0, 0) == DigiviceFormTagdrake);
}

static void test_evolution_waits_for_ack(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormBitling;
    s.exp = DIGIVICE_CHAMPION_EXP;
    CHECK(digivice_rules_evolve(&s, T0));
    CHECK(s.form == DigiviceFormBytemon);
    CHECK(s.evo_from == DigiviceFormBitling);
    CHECK(!digivice_rules_evolve(&s, T0));
    CHECK(s.form == DigiviceFormBytemon);
    CHECK(s.evo_from == DigiviceFormBitling);
    digivice_rules_ack_evolution(&s);
    CHECK(digivice_rules_evolve(&s, T0));
    CHECK(s.evo_from == DigiviceFormBytemon);
}

static void test_champion_and_corrupt_do_not_evolve(void) {
    DigiviceState s = rookie(T0);
    s.form = DigiviceFormTagdrake;
    s.exp = 99999;
    CHECK(!digivice_rules_evolve(&s, T0));
    s.form = DigiviceFormVirubyte;
    CHECK(!digivice_rules_evolve(&s, T0));
}

static void test_deed_types(void) {
    CHECK(digivice_rules_deed_type(DolphinDeedNfcRead) == DigiviceTypeNfc);
    CHECK(digivice_rules_deed_type(DolphinDeedNfcAddEmulate) == DigiviceTypeNfc);
    CHECK(digivice_rules_deed_type(DolphinDeedRfidReadSuccess) == DigiviceTypeNfc);
    CHECK(digivice_rules_deed_type(DolphinDeedIbuttonEmulate) == DigiviceTypeNfc);
    CHECK(digivice_rules_deed_type(DolphinDeedSubGhzReceiverInfo) == DigiviceTypeSubGhz);
    CHECK(digivice_rules_deed_type(DolphinDeedSubGhzFrequencyAnalyzer) == DigiviceTypeSubGhz);
    CHECK(digivice_rules_deed_type(DolphinDeedIrSend) == DigiviceTypeIr);
    CHECK(digivice_rules_deed_type(DolphinDeedIrSave) == DigiviceTypeIr);
    CHECK(digivice_rules_deed_type(DolphinDeedBadUsbPlayScript) == DigiviceTypeUsb);
    CHECK(digivice_rules_deed_type(DolphinDeedU2fAuthorized) == DigiviceTypeUsb);
    CHECK(digivice_rules_deed_type(DolphinDeedGpioUartBridge) == DigiviceTypeUsb);
    CHECK(digivice_rules_deed_type(DolphinDeedPluginStart) == DigiviceTypeNone);
    CHECK(digivice_rules_deed_type(DolphinDeedPluginGameStart) == DigiviceTypeNone);
    CHECK(digivice_rules_deed_type(DolphinDeedPluginGameWin) == DigiviceTypeNone);
    CHECK(digivice_rules_deed_type(DolphinDeedTestLeft) == DigiviceTypeNone);
}

static void test_deed_grants_exp_and_data(void) {
    DigiviceState s = hacker(fresh(T0));
    digivice_rules_on_deed(&s, DolphinDeedIrLearnSuccess, 3, T0, 99);
    CHECK(s.exp == 3);
    CHECK(s.data[DigiviceTypeIr] == 1);
    digivice_rules_on_deed(&s, DolphinDeedPluginGameWin, 10, T0, 99);
    CHECK(s.exp == 13);
    CHECK(
        s.data[DigiviceTypeNfc] + s.data[DigiviceTypeSubGhz] + s.data[DigiviceTypeIr] +
            s.data[DigiviceTypeUsb] ==
        1);
    digivice_rules_on_deed(&s, DolphinDeedTestRight, 5, T0, 0);
    CHECK(s.exp == 13);
}

static void test_spawn_rules(void) {
    CHECK(digivice_rules_is_spawn_deed(DolphinDeedNfcRead));
    CHECK(digivice_rules_is_spawn_deed(DolphinDeedRfidRead));
    CHECK(digivice_rules_is_spawn_deed(DolphinDeedIbuttonRead));
    CHECK(digivice_rules_is_spawn_deed(DolphinDeedSubGhzReceiverInfo));
    CHECK(digivice_rules_is_spawn_deed(DolphinDeedSubGhzFrequencyAnalyzer));
    CHECK(!digivice_rules_is_spawn_deed(DolphinDeedNfcReadSuccess));
    CHECK(!digivice_rules_is_spawn_deed(DolphinDeedIrSend));

    DigiviceState baby = hacker(fresh(T0));
    baby.form = DigiviceFormBitling;
    CHECK(!digivice_rules_on_deed(&baby, DolphinDeedNfcRead, 1, T0, 0));

    DigiviceState s = hacker(rookie(T0));
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedIrSend, 1, T0, 0));
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedNfcRead, 1, T0, DIGIVICE_SPAWN_PCT));
    CHECK(digivice_rules_on_deed(&s, DolphinDeedRfidRead, 1, T0, DIGIVICE_SPAWN_PCT - 1));
    CHECK(s.virux_type == DigiviceTypeNfc);
    CHECK(s.virux_expires == T0 + DIGIVICE_VIRUX_TTL_S);

    // At most one pending.
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedSubGhzReceiverInfo, 1, T0 + 60, 0));
    CHECK(s.virux_type == DigiviceTypeNfc);

    CHECK(digivice_rules_virux_active(&s, T0 + DIGIVICE_VIRUX_TTL_S - 1));
    CHECK(!digivice_rules_virux_active(&s, T0 + DIGIVICE_VIRUX_TTL_S));
    digivice_rules_tick(&s, T0 + DIGIVICE_VIRUX_TTL_S);
    CHECK(s.virux_type == DigiviceTypeNone);

    DigiviceState c = hacker(rookie(T0));
    c.form = DigiviceFormVirubyte;
    c.prev_form = DigiviceFormBytemon;
    CHECK(digivice_rules_on_deed(&c, DolphinDeedSubGhzFrequencyAnalyzer, 1, T0, 0));
    CHECK(c.virux_type == DigiviceTypeSubGhz);
}

static void test_player_type_by_form(void) {
    DigiviceState s = rookie(T0);
    CHECK(digivice_rules_player_type(&s) == DigiviceTypeNone);
    s.form = DigiviceFormTagdrake;
    CHECK(digivice_rules_player_type(&s) == DigiviceTypeNfc);
    s.form = DigiviceFormPlugdrake;
    CHECK(digivice_rules_player_type(&s) == DigiviceTypeUsb);
    s.form = DigiviceFormVirubyte;
    s.prev_form = DigiviceFormPlugdrake;
    CHECK(digivice_rules_player_type(&s) == DigiviceTypeNone);
    CHECK(digivice_rules_stage(&s) == DigiviceStageChampion);
}

static void test_lose_can_transform(void) {
    DigiviceState s = rookie(T0);
    s.virux_type = DigiviceTypeIr;
    s.virux_expires = T0 + 60;
    s.corruption = 90;
    digivice_rules_battle_result(&s, DigiviceBattleResultLose, T0);
    CHECK(s.form == DigiviceFormVirubyte);
    CHECK(s.prev_form == DigiviceFormBytemon);
}

static void test_type_chart(void) {
    CHECK(digivice_rules_type_x10(DigiviceTypeNfc, DigiviceTypeUsb) == 15);
    CHECK(digivice_rules_type_x10(DigiviceTypeUsb, DigiviceTypeIr) == 15);
    CHECK(digivice_rules_type_x10(DigiviceTypeIr, DigiviceTypeSubGhz) == 15);
    CHECK(digivice_rules_type_x10(DigiviceTypeSubGhz, DigiviceTypeNfc) == 15);
    CHECK(digivice_rules_type_x10(DigiviceTypeUsb, DigiviceTypeNfc) == 10);
    CHECK(digivice_rules_type_x10(DigiviceTypeNfc, DigiviceTypeNfc) == 10);
    CHECK(digivice_rules_type_x10(DigiviceTypeNone, DigiviceTypeNfc) == 10);
    CHECK(digivice_rules_type_x10(DigiviceTypeNfc, DigiviceTypeNone) == 10);
}

static void test_damage_formula(void) {
    // (base + strength) x timing x type, integer math.
    CHECK(digivice_rules_damage(4, 0, true, DigiviceTypeNone, DigiviceTypeNfc) == 6);
    CHECK(digivice_rules_damage(4, 0, false, DigiviceTypeNone, DigiviceTypeNfc) == 2);
    CHECK(digivice_rules_damage(4, 6, true, DigiviceTypeNfc, DigiviceTypeUsb) == 22);
    CHECK(digivice_rules_damage(7, 3, false, DigiviceTypeIr, DigiviceTypeSubGhz) == 7);
    CHECK(digivice_rules_damage(1, 0, false, DigiviceTypeNone, DigiviceTypeNone) == 1);
}

static void test_battle_flow(void) {
    DigiviceState s = rookie(T0);
    s.virux_type = DigiviceTypeIr;
    s.virux_expires = T0 + 60;
    DigiviceBattle b;
    digivice_battle_init(&b, &s);
    CHECK(b.stage == DigiviceStageRookie);
    CHECK(b.foe_type == DigiviceTypeIr);
    CHECK(b.player_hp == b.player_hp_max && b.foe_hp == b.foe_hp_max);
    CHECK(digivice_battle_outcome(&b) == DigiviceBattleOngoing);

    uint16_t foe_before = b.foe_hp;
    uint16_t dealt = digivice_battle_player_attack(&b, &s, true, 99);
    CHECK(dealt == 6);
    CHECK(b.foe_hp == foe_before - 6);

    uint16_t taken = digivice_battle_foe_attack(&b);
    digivice_battle_defend(&b);
    CHECK(b.defending);
    uint16_t halved = digivice_battle_foe_attack(&b);
    CHECK(halved == taken / 2);
    CHECK(!b.defending);

    b.foe_hp = 1;
    digivice_battle_player_attack(&b, &s, false, 99);
    CHECK(digivice_battle_outcome(&b) == DigiviceBattleWon);

    b.foe_hp = 5;
    b.player_hp = 1;
    digivice_battle_foe_attack(&b);
    CHECK(digivice_battle_outcome(&b) == DigiviceBattleLost);
}

static void test_corrupt_attack_can_miss(void) {
    DigiviceState s = rookie(T0);
    s.corruption = 60;
    s.virux_type = DigiviceTypeNfc;
    DigiviceBattle b;
    digivice_battle_init(&b, &s);
    uint16_t hp = b.foe_hp;
    CHECK(digivice_battle_player_attack(&b, &s, true, 0) == 0);
    CHECK(b.foe_hp == hp);
}

static void test_flee(void) {
    CHECK(digivice_battle_flee(DIGIVICE_FLEE_PCT - 1));
    CHECK(!digivice_battle_flee(DIGIVICE_FLEE_PCT));
}

static void test_battle_rewards(void) {
    DigiviceState s = rookie(T0);
    s.virux_type = DigiviceTypeUsb;
    s.virux_expires = T0 + 60;
    uint32_t exp = s.exp;
    digivice_rules_battle_result(&s, DigiviceBattleResultWin, T0);
    CHECK(s.exp == exp + 2 * DIGIVICE_WIN_BASE_EXP);
    CHECK(s.data[DigiviceTypeUsb] == DIGIVICE_WIN_DATA);
    CHECK(s.wins == 1);
    CHECK(s.virux_type == DigiviceTypeNone);

    s.virux_type = DigiviceTypeNfc;
    digivice_rules_battle_result(&s, DigiviceBattleResultLose, T0);
    CHECK(s.corruption == DIGIVICE_CORRUPTION_LOSE_GAIN);
    CHECK(s.losses == 1);
    CHECK(s.virux_type == DigiviceTypeNone);

    s.virux_type = DigiviceTypeIr;
    DigiviceState before = s;
    digivice_rules_battle_result(&s, DigiviceBattleResultFled, T0);
    CHECK(s.virux_type == DigiviceTypeNone);
    CHECK(s.exp == before.exp && s.corruption == before.corruption);
}

static void test_corrupt_pet_fights_at_pre_corruption_stage(void) {
    DigiviceState champ = fresh(T0);
    champ.form = DigiviceFormVirubyte;
    champ.prev_form = DigiviceFormTagdrake;
    champ.corruption = DIGIVICE_CORRUPTION_MAX;
    champ.virux_type = DigiviceTypeIr;
    DigiviceBattle b;
    digivice_battle_init(&b, &champ);
    const uint16_t champion_hp[] = DIGIVICE_PLAYER_HP;
    const uint16_t champion_foe_hp[] = DIGIVICE_VIRUX_HP;
    CHECK(b.stage == DigiviceStageChampion);
    CHECK(b.player_hp == champion_hp[DigiviceStageChampion]);
    CHECK(b.player_hp_max == champion_hp[DigiviceStageChampion]);
    CHECK(b.foe_hp == champion_foe_hp[DigiviceStageChampion]);
    // A corrupt pet has no type of its own.
    CHECK(b.player_type == DigiviceTypeNone);

    // Same shape from a Rookie, so a stage() that ignores prev_form cannot pass both.
    DigiviceState rook = champ;
    rook.prev_form = DigiviceFormBytemon;
    digivice_battle_init(&b, &rook);
    CHECK(b.stage == DigiviceStageRookie);
    CHECK(b.player_hp == champion_hp[DigiviceStageRookie]);
    CHECK(b.foe_hp == champion_foe_hp[DigiviceStageRookie]);
}

static void test_corrupt_attack_miss_edges(void) {
    DigiviceState s = rookie(T0);
    s.virux_type = DigiviceTypeNfc;
    DigiviceBattle b;

    s.corruption = DIGIVICE_CORRUPTION_GLITCH;
    digivice_battle_init(&b, &s);
    uint16_t hp = b.foe_hp;
    CHECK(digivice_battle_player_attack(&b, &s, true, DIGIVICE_CORRUPTION_MISS_PCT - 1) == 0);
    CHECK(b.foe_hp == hp);
    CHECK(digivice_battle_player_attack(&b, &s, true, DIGIVICE_CORRUPTION_MISS_PCT) > 0);
    CHECK(b.foe_hp < hp);

    // Just under the glitch threshold the roll never matters.
    s.corruption = DIGIVICE_CORRUPTION_GLITCH - 1;
    digivice_battle_init(&b, &s);
    for(uint8_t rng = 0; rng < 100; rng++) {
        CHECK(digivice_battle_player_attack(&b, &s, false, rng) > 0);
    }
}

static void test_foe_type_multiplier(void) {
    const uint16_t atk[] = DIGIVICE_VIRUX_ATK;
    uint16_t adv = atk[DigiviceStageChampion] * DIGIVICE_TYPE_ADV_X10 / 10;
    CHECK(adv != atk[DigiviceStageChampion]);

    // USB beats IR, so a USB foe has the advantage over an IR player.
    DigiviceState ir = fresh(T0);
    ir.form = DigiviceFormBeamdrake;
    ir.virux_type = DigiviceTypeUsb;
    DigiviceBattle b;
    digivice_battle_init(&b, &ir);
    CHECK(digivice_battle_foe_attack(&b) == adv);
    CHECK(b.player_hp == b.player_hp_max - adv);

    // The advantage also stacks with defending: halved after the multiplier.
    digivice_battle_init(&b, &ir);
    digivice_battle_defend(&b);
    CHECK(digivice_battle_foe_attack(&b) == adv / 2);

    // The reverse matchup (IR foe vs USB player) and same-type are neutral.
    DigiviceState usb = fresh(T0);
    usb.form = DigiviceFormPlugdrake;
    usb.virux_type = DigiviceTypeIr;
    digivice_battle_init(&b, &usb);
    CHECK(digivice_battle_foe_attack(&b) == atk[DigiviceStageChampion]);

    usb.virux_type = DigiviceTypeUsb;
    digivice_battle_init(&b, &usb);
    CHECK(digivice_battle_foe_attack(&b) == atk[DigiviceStageChampion]);
}

static void test_win_without_virux_type_grants_exp_only(void) {
    // Guard bytes after the state catch a write through data[DigiviceTypeNone].
    struct {
        DigiviceState s;
        uint8_t guard[1024];
    } box;
    box.s = rookie(T0);
    memset(box.guard, 0xAA, sizeof(box.guard));
    uint32_t exp = box.s.exp;
    CHECK(box.s.virux_type == DigiviceTypeNone);

    digivice_rules_battle_result(&box.s, DigiviceBattleResultWin, T0);
    CHECK(box.s.exp == exp + 2 * DIGIVICE_WIN_BASE_EXP);
    CHECK(box.s.wins == 1);
    for(uint8_t t = 0; t < DigiviceTypeCount; t++) {
        CHECK(box.s.data[t] == 0);
    }
    for(size_t i = 0; i < sizeof(box.guard); i++) {
        if(box.guard[i] != 0xAA) {
            CHECK(box.guard[i] == 0xAA);
            break;
        }
    }
}

static void test_deed_out_of_range_is_ignored(void) {
    DigiviceState s = hacker(rookie(T0));
    DigiviceState before = s;
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedMAX, 5, T0, 0));
    CHECK(!digivice_rules_on_deed(&s, (DolphinDeed)(DolphinDeedMAX + 1), 5, T0, 0));
    CHECK(!digivice_rules_on_deed(&s, (DolphinDeed)0xFF, 5, T0, 0));
    CHECK(memcmp(&s, &before, sizeof(s)) == 0);
}

static void test_spawn_deed_when_ineligible_still_rewards(void) {
    // Baby: eligible deed, wrong stage.
    DigiviceState baby = hacker(fresh(T0));
    baby.form = DigiviceFormBitling;
    CHECK(!digivice_rules_on_deed(&baby, DolphinDeedNfcRead, 2, T0, 0));
    CHECK(baby.exp == 2);
    CHECK(baby.data[DigiviceTypeNfc] == 1);
    CHECK(baby.virux_type == DigiviceTypeNone);

    // Rookie with a VIRUX already pending: neither type nor timer may be overwritten.
    DigiviceState s = hacker(rookie(T0));
    s.virux_type = DigiviceTypeIr;
    s.virux_expires = T0 + 100;
    uint32_t exp = s.exp;
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedNfcRead, 2, T0, 0));
    CHECK(s.exp == exp + 2);
    CHECK(s.data[DigiviceTypeNfc] == 1);
    CHECK(s.virux_type == DigiviceTypeIr);
    CHECK(s.virux_expires == T0 + 100);
}

static void test_v2_defaults_and_layout(void) {
    DigiviceState s = fresh(T0);
    CHECK(s.lang == DigiviceLangEs);
    CHECK(s.mode == DigiviceModeNormal);
    CHECK(s.light_on == 1);
    CHECK(s.poop == 0 && s.sick == 0 && s.care_mistakes == 0 && s.doses_needed == 0);
    CHECK(s.poop_acc_s == 0 && s.poop_dirty_since == 0 && s.starving_since == 0);
    CHECK(s.sick_since == 0 && s.light_on_asleep_s == 0);
    // The v1 file is 56 bytes; v2 appends 28 and keeps every v1 offset.
    CHECK(sizeof(DigiviceStateV1) == 56);
    CHECK(sizeof(DigiviceState) == 84);
    CHECK(offsetof(DigiviceState, corrupt_acc_s) == offsetof(DigiviceStateV1, corrupt_acc_s));
}

static void test_normal_mode_ignores_deeds(void) {
    DigiviceState s = rookie(T0);
    CHECK(s.mode == DigiviceModeNormal);
    DigiviceState before = s;
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedNfcRead, 5, T0, 0));
    CHECK(!digivice_rules_on_deed(&s, DolphinDeedIrSend, 5, T0, 0));
    CHECK(memcmp(&s, &before, sizeof(s)) == 0);
}

static void test_train_grants_exp_in_both_modes(void) {
    for(uint8_t mode = 0; mode < DigiviceModeCount; mode++) {
        DigiviceState s = rookie(T0);
        s.mode = mode;
        uint32_t exp = s.exp;
        CHECK(digivice_rules_train(&s, true, T0) == DigiviceResultOk);
        CHECK(s.exp == exp + DIGIVICE_TRAIN_EXP_OK);
        CHECK(digivice_rules_train(&s, false, T0) == DigiviceResultOk);
        CHECK(s.exp == exp + DIGIVICE_TRAIN_EXP_OK + DIGIVICE_TRAIN_EXP_FAIL);
    }
}

static void test_train_refused_while_sick_or_asleep(void) {
    DigiviceState s = rookie(T0);
    s.sick = 1;
    DigiviceState before = s;
    CHECK(digivice_rules_can_train(&s, T0) == DigiviceResultSick);
    CHECK(digivice_rules_train(&s, true, T0) == DigiviceResultSick);
    CHECK(memcmp(&s, &before, sizeof(s)) == 0);

    // 23:30 is inside the sleep window; sickness wins when both apply.
    uint32_t night = DAY(10) + H(23) + 1800;
    CHECK(digivice_rules_can_train(&s, night) == DigiviceResultSick);
    s.sick = 0;
    before = s;
    CHECK(digivice_rules_train(&s, true, night) == DigiviceResultAsleep);
    CHECK(memcmp(&s, &before, sizeof(s)) == 0);
    CHECK(digivice_rules_can_train(&s, T0) == DigiviceResultOk);
}

static void test_battle_refused_while_sick(void) {
    DigiviceState s = rookie(T0);
    CHECK(digivice_rules_can_battle(&s) == DigiviceResultOk);
    s.sick = 1;
    CHECK(digivice_rules_can_battle(&s) == DigiviceResultSick);
}

static void test_migrate_v1_field_by_field(void) {
    DigiviceStateV1 v1 = {
        .form = DigiviceFormWavedrake,
        .prev_form = DigiviceFormBytemon,
        .evo_from = DigiviceFormBytemon,
        .hunger = 3,
        .mood = 2,
        .corruption = 40,
        .virux_type = DigiviceTypeIr,
        .reserved = 0,
        .strength = 11,
        .wins = 12,
        .losses = 13,
        .data = {21, 22, 23, 24},
        .exp = 345,
        .virux_expires = T0 + 100,
        .last_tick = T0 + 200,
        .born_at = T0 - DAY(5),
        .clean_since = T0 - H(3),
        .hunger_acc_s = 1001,
        .mood_acc_s = 1002,
        .corrupt_acc_s = 1003,
    };
    DigiviceState s;
    memset(&s, 0xAA, sizeof(s));
    digivice_rules_migrate_v1(&v1, &s);
    CHECK(s.form == DigiviceFormWavedrake);
    CHECK(s.prev_form == DigiviceFormBytemon);
    CHECK(s.evo_from == DigiviceFormBytemon);
    CHECK(s.hunger == 3 && s.mood == 2 && s.corruption == 40);
    CHECK(s.virux_type == DigiviceTypeIr && s.reserved == 0);
    CHECK(s.strength == 11 && s.wins == 12 && s.losses == 13);
    CHECK(s.data[0] == 21 && s.data[1] == 22 && s.data[2] == 23 && s.data[3] == 24);
    CHECK(s.exp == 345);
    CHECK(s.virux_expires == T0 + 100 && s.last_tick == T0 + 200);
    CHECK(s.born_at == T0 - DAY(5) && s.clean_since == T0 - H(3));
    CHECK(s.hunger_acc_s == 1001 && s.mood_acc_s == 1002 && s.corrupt_acc_s == 1003);
    CHECK(s.lang == DigiviceLangEs && s.mode == DigiviceModeNormal && s.light_on == 1);
    CHECK(s.poop == 0 && s.sick == 0 && s.care_mistakes == 0 && s.doses_needed == 0);
    CHECK(s.reserved2 == 0 && s.poop_acc_s == 0 && s.poop_dirty_since == 0);
    CHECK(s.starving_since == 0 && s.sick_since == 0 && s.light_on_asleep_s == 0);
}

static void test_lang_and_mode_are_range_checked(void) {
    DigiviceState s = fresh(T0);
    CHECK(digivice_rules_set_lang(&s, DigiviceLangEn) && s.lang == DigiviceLangEn);
    CHECK(!digivice_rules_set_lang(&s, DigiviceLangCount) && s.lang == DigiviceLangEn);
    CHECK(digivice_rules_set_mode(&s, DigiviceModeHacker) && s.mode == DigiviceModeHacker);
    CHECK(!digivice_rules_set_mode(&s, 0xFF) && s.mode == DigiviceModeHacker);

    s.lang = 9;
    s.mode = 9;
    digivice_rules_sanitize(&s);
    CHECK(s.lang == DigiviceLangEs && s.mode == DigiviceModeNormal);
    s.lang = DigiviceLangEn;
    s.mode = DigiviceModeHacker;
    digivice_rules_sanitize(&s);
    CHECK(s.lang == DigiviceLangEn && s.mode == DigiviceModeHacker);
}

static void test_reset_keeps_lang_and_mode(void) {
    DigiviceState s = rookie(T0);
    s.lang = DigiviceLangEn;
    s.mode = DigiviceModeHacker;
    s.exp = 999;
    digivice_rules_reset(&s, T0 + 50);
    CHECK(s.form == DigiviceFormByteegg && s.exp == 0 && s.born_at == T0 + 50);
    CHECK(s.lang == DigiviceLangEn && s.mode == DigiviceModeHacker);
}

static void test_same_visible_ignores_care_bookkeeping(void) {
    DigiviceState a = fresh(T0);
    DigiviceState b = a;
    b.poop_acc_s += 60;
    b.light_on_asleep_s += 60;
    CHECK(digivice_rules_same_visible(&a, &b));
    b.poop++;
    CHECK(!digivice_rules_same_visible(&a, &b));
}

static DigiviceState baby(uint32_t now) {
    DigiviceState s = fresh(now);
    s.form = DigiviceFormBitling;
    return s;
}

static void test_poop_every_3h_awake(void) {
    DigiviceState s = baby(T0);
    digivice_rules_tick(&s, T0 + H(3) - 1);
    CHECK(s.poop == 0);
    digivice_rules_tick(&s, T0 + H(3));
    CHECK(s.poop == 1);
    CHECK(s.poop_dirty_since == T0 + H(3));
    digivice_rules_tick(&s, T0 + H(6));
    CHECK(s.poop == 2);
    // The oldest poop keeps the timestamp.
    CHECK(s.poop_dirty_since == T0 + H(3));
}

static void test_poop_pauses_while_asleep(void) {
    // 22:00 -> 09:00: one awake hour before 23:00 and two after 07:00.
    uint32_t start = DAY(10) + H(22);
    DigiviceState s = baby(start);
    digivice_rules_tick(&s, DAY(11) + H(9) - 1);
    CHECK(s.poop == 0);
    digivice_rules_tick(&s, DAY(11) + H(9));
    CHECK(s.poop == 1);
    CHECK(s.poop_dirty_since == DAY(11) + H(9));
}

static void test_poop_is_capped(void) {
    DigiviceState s = baby(T0);
    s.poop = DIGIVICE_POOP_MAX;
    s.poop_dirty_since = T0;
    digivice_rules_tick(&s, T0 + H(3));
    CHECK(s.poop == DIGIVICE_POOP_MAX);
    CHECK(s.poop_acc_s == 0);
}

static void test_egg_never_poops(void) {
    DigiviceState s = fresh(T0);
    digivice_rules_tick(&s, T0 + H(9));
    CHECK(s.poop == 0);
    CHECK(s.poop_acc_s == 0);
}

static void test_mood_decays_twice_as_fast_with_3_poops(void) {
    DigiviceState s = baby(T0);
    s.poop = DIGIVICE_POOP_MOOD_FAST;
    s.poop_dirty_since = T0;
    digivice_rules_tick(&s, T0 + DIGIVICE_MOOD_DECAY_S / 2 - 1);
    CHECK(s.mood == DIGIVICE_MOOD_MAX);
    digivice_rules_tick(&s, T0 + DIGIVICE_MOOD_DECAY_S / 2);
    CHECK(s.mood == DIGIVICE_MOOD_MAX - 1);

    // Two poops are still the normal rate.
    DigiviceState n = baby(T0);
    n.poop = DIGIVICE_POOP_MOOD_FAST - 1;
    n.poop_dirty_since = T0;
    digivice_rules_tick(&n, T0 + DIGIVICE_MOOD_DECAY_S - 1);
    CHECK(n.mood == DIGIVICE_MOOD_MAX);
}

static void test_dirty_poop_is_one_mistake_per_6h(void) {
    DigiviceState s = baby(T0);
    s.poop = 1;
    s.poop_dirty_since = T0;
    digivice_rules_tick(&s, T0 + H(6) - 1);
    CHECK(s.care_mistakes == 0);
    digivice_rules_tick(&s, T0 + H(6));
    CHECK(s.care_mistakes == 1);
    CHECK(s.poop_dirty_since == T0 + H(6));
    digivice_rules_tick(&s, T0 + H(12) - 1);
    CHECK(s.care_mistakes == 1);
}

static void test_clean(void) {
    DigiviceState s = baby(T0);
    s.poop = 3;
    s.poop_dirty_since = T0 - H(1);
    s.mood = 2;
    CHECK(digivice_rules_clean(&s, T0));
    CHECK(s.poop == 0 && s.poop_dirty_since == 0 && s.mood == 3);

    DigiviceState before = s;
    CHECK(!digivice_rules_clean(&s, T0));
    CHECK(memcmp(&s, &before, sizeof(s)) == 0);

    s.poop = 1;
    s.mood = DIGIVICE_MOOD_MAX;
    CHECK(digivice_rules_clean(&s, T0));
    CHECK(s.mood == DIGIVICE_MOOD_MAX);
}

static void test_60s_ticks_lose_no_poop(void) {
    // 08:00 -> 20:00, all awake: four poops land on the same seconds either way.
    DigiviceState a = baby(T0);
    DigiviceState b = a;
    for(uint32_t t = T0 + 60; t <= T0 + H(12); t += 60)
        digivice_rules_tick(&a, t);
    digivice_rules_tick(&b, T0 + H(12));
    CHECK(a.poop == 4);
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
}

#define NIGHT(day) (DAY(day) + H(23))

static void test_sick_from_two_mistakes(void) {
    DigiviceState s = baby(T0);
    s.care_mistakes = 1;
    s.poop = 1;
    s.poop_dirty_since = T0;
    digivice_rules_tick(&s, T0 + H(6) - 1);
    CHECK(!s.sick);
    digivice_rules_tick(&s, T0 + H(6));
    CHECK(s.care_mistakes == DIGIVICE_SICK_MISTAKES);
    CHECK(s.sick == 1 && s.doses_needed == 1 && s.sick_since == T0 + H(6));
}

static void test_sick_from_24h_of_starvation(void) {
    // An egg has no poop or light mistakes, so only starvation can make it sick.
    DigiviceState s = fresh(T0);
    s.hunger = 1;
    digivice_rules_tick(&s, T0 + H(2));
    CHECK(s.hunger == 0 && s.starving_since == T0 + H(2));
    // Wall time: the night in between counts.
    digivice_rules_tick(&s, T0 + H(26) - 1);
    CHECK(!s.sick);
    digivice_rules_tick(&s, T0 + H(26));
    CHECK(s.sick == 1 && s.sick_since == T0 + H(26));
}

static void test_feeding_stops_the_starvation_clock(void) {
    DigiviceState s = fresh(T0);
    s.hunger = 0;
    digivice_rules_tick(&s, T0 + 60);
    CHECK(s.starving_since == T0);
    CHECK(digivice_rules_feed(&s, T0 + 120));
    CHECK(s.starving_since == 0);

    // Training can starve it again.
    digivice_rules_train(&s, false, T0 + 180);
    CHECK(s.hunger == 0 && s.starving_since == T0 + 180);
}

static void test_sickness_doubles_mood_decay_and_corrupts(void) {
    DigiviceState s = fresh(T0);
    s.sick = 1;
    s.doses_needed = 1;
    s.sick_since = T0;
    digivice_rules_tick(&s, T0 + H(1));
    CHECK(s.corruption == DIGIVICE_CORRUPTION_PER_HOUR);
    CHECK(s.hunger > 0 && s.mood > 0);
    digivice_rules_tick(&s, T0 + DIGIVICE_MOOD_DECAY_S / 2);
    CHECK(s.mood == DIGIVICE_MOOD_MAX - 1);
}

static void test_second_dose_after_12h(void) {
    DigiviceState s = fresh(T0);
    s.sick = 1;
    s.doses_needed = 1;
    s.sick_since = T0;
    for(uint32_t t = T0 + 60; t < T0 + H(12); t += 60)
        digivice_rules_tick(&s, t);
    CHECK(s.doses_needed == 1);
    digivice_rules_tick(&s, T0 + H(12));
    CHECK(s.doses_needed == 2);

    CHECK(digivice_rules_cure(&s, T0 + H(13)));
    CHECK(s.sick == 1 && s.doses_needed == 1);
    // The 12 h mark is behind us: one remaining dose stays one.
    digivice_rules_tick(&s, T0 + H(14));
    CHECK(s.doses_needed == 1);
    CHECK(digivice_rules_cure(&s, T0 + H(14)));
    CHECK(s.sick == 0 && s.doses_needed == 0);
}

static void test_cure_resets_mistakes(void) {
    DigiviceState s = baby(T0);
    s.sick = 1;
    s.doses_needed = 1;
    s.sick_since = T0 - H(1);
    s.care_mistakes = 3;
    CHECK(digivice_rules_cure(&s, T0));
    CHECK(s.sick == 0 && s.care_mistakes == 0 && s.sick_since == 0 && s.doses_needed == 0);
}

static void test_cure_refused_when_healthy(void) {
    DigiviceState s = baby(T0);
    s.care_mistakes = 1;
    DigiviceState before = s;
    CHECK(!digivice_rules_cure(&s, T0));
    CHECK(memcmp(&s, &before, sizeof(s)) == 0);
}

static void test_cure_restarts_the_starvation_clock(void) {
    DigiviceState s = fresh(T0);
    s.hunger = 0;
    s.starving_since = T0 - H(30);
    s.sick = 1;
    s.doses_needed = 1;
    s.sick_since = T0 - H(6);
    CHECK(digivice_rules_cure(&s, T0));
    CHECK(s.starving_since == T0);
    s.last_tick = T0;
    digivice_rules_tick(&s, T0 + 60);
    CHECK(!s.sick);
}

static void test_light_on_asleep_is_one_mistake_per_window(void) {
    DigiviceState s = baby(NIGHT(10));
    digivice_rules_tick(&s, NIGHT(10) + H(1) - 1);
    CHECK(s.care_mistakes == 0 && s.mood == DIGIVICE_MOOD_MAX);
    digivice_rules_tick(&s, NIGHT(10) + H(1));
    CHECK(s.care_mistakes == 1 && s.mood == DIGIVICE_MOOD_MAX - 1);
    digivice_rules_tick(&s, DAY(11) + H(7) - 1);
    CHECK(s.care_mistakes == 1);
}

static void test_light_resets_at_7(void) {
    DigiviceState s = baby(NIGHT(10));
    CHECK(digivice_rules_toggle_light(&s, NIGHT(10)) == DigiviceResultOk);
    CHECK(s.light_on == 0);
    digivice_rules_tick(&s, DAY(11) + H(7) - 1);
    CHECK(s.light_on == 0 && s.care_mistakes == 0);
    digivice_rules_tick(&s, DAY(11) + H(7) + 60);
    CHECK(s.light_on == 1 && s.light_on_asleep_s == 0);
}

static void test_light_toggled_twice_counts_once(void) {
    DigiviceState s = baby(NIGHT(10));
    digivice_rules_tick(&s, NIGHT(10) + 1800);
    CHECK(digivice_rules_toggle_light(&s, NIGHT(10) + 1800) == DigiviceResultOk);
    digivice_rules_tick(&s, NIGHT(10) + 3600);
    CHECK(digivice_rules_toggle_light(&s, NIGHT(10) + 3600) == DigiviceResultOk);
    digivice_rules_tick(&s, NIGHT(10) + 5400 - 1);
    CHECK(s.care_mistakes == 0);
    digivice_rules_tick(&s, NIGHT(10) + 5400);
    CHECK(s.care_mistakes == 1);
    digivice_rules_tick(&s, DAY(11) + H(6));
    CHECK(s.care_mistakes == 1);
}

static void test_light_refused_while_awake(void) {
    DigiviceState s = baby(T0);
    DigiviceState before = s;
    CHECK(digivice_rules_toggle_light(&s, T0) == DigiviceResultAwake);
    CHECK(memcmp(&s, &before, sizeof(s)) == 0);
    CHECK(s.light_on == 1);
}

static void test_60s_ticks_match_one_tick_with_care(void) {
    // 30 h from 08:00 across a night: poop, starvation, mistakes, sickness and the light.
    DigiviceState a = baby(T0);
    a.hunger = 1;
    DigiviceState b = a;
    for(uint32_t t = T0 + 60; t <= T0 + H(30); t += 60)
        digivice_rules_tick(&a, t);
    digivice_rules_tick(&b, T0 + H(30));
    CHECK(a.sick == 1 && a.care_mistakes >= DIGIVICE_SICK_MISTAKES);
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
}

int main(void) {
    test_init();
    test_hunger_decays_every_2h();
    test_mood_decays_every_3h();
    test_60s_ticks_lose_nothing();
    test_sleep_window();
    test_elapsed_clamped_to_48h();
    test_clock_backwards_is_zero_elapsed();
    test_corruption_grows_5_per_hour_at_zero();
    test_corruption_starts_when_hunger_hits_zero();
    test_feed();
    test_train();
    test_miss_chance_from_50();
    test_transform_at_100_saves_form();
    test_baby_never_transforms();
    test_recovery_after_24h_clean();
    test_recovery_resets_when_corruption_returns();
    test_timing_zone();
    test_same_visible_ignores_bookkeeping();
    test_egg_hatches_after_300s();
    test_baby_to_rookie_at_50_exp();
    test_champion_paths_and_ties();
    test_evolution_waits_for_ack();
    test_champion_and_corrupt_do_not_evolve();
    test_deed_types();
    test_deed_grants_exp_and_data();
    test_spawn_rules();
    test_player_type_by_form();
    test_lose_can_transform();
    test_type_chart();
    test_damage_formula();
    test_battle_flow();
    test_corrupt_attack_can_miss();
    test_flee();
    test_battle_rewards();
    test_corrupt_pet_fights_at_pre_corruption_stage();
    test_corrupt_attack_miss_edges();
    test_foe_type_multiplier();
    test_win_without_virux_type_grants_exp_only();
    test_deed_out_of_range_is_ignored();
    test_spawn_deed_when_ineligible_still_rewards();
    test_v2_defaults_and_layout();
    test_normal_mode_ignores_deeds();
    test_train_grants_exp_in_both_modes();
    test_train_refused_while_sick_or_asleep();
    test_battle_refused_while_sick();
    test_migrate_v1_field_by_field();
    test_lang_and_mode_are_range_checked();
    test_reset_keeps_lang_and_mode();
    test_same_visible_ignores_care_bookkeeping();
    test_poop_every_3h_awake();
    test_poop_pauses_while_asleep();
    test_poop_is_capped();
    test_egg_never_poops();
    test_mood_decays_twice_as_fast_with_3_poops();
    test_dirty_poop_is_one_mistake_per_6h();
    test_clean();
    test_60s_ticks_lose_no_poop();
    test_sick_from_two_mistakes();
    test_sick_from_24h_of_starvation();
    test_feeding_stops_the_starvation_clock();
    test_sickness_doubles_mood_decay_and_corrupts();
    test_second_dose_after_12h();
    test_cure_resets_mistakes();
    test_cure_refused_when_healthy();
    test_cure_restarts_the_starvation_clock();
    test_light_on_asleep_is_one_mistake_per_window();
    test_light_resets_at_7();
    test_light_toggled_twice_counts_once();
    test_light_refused_while_awake();
    test_60s_ticks_match_one_tick_with_care();

    if(failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("digivice_rules: all checks passed\n");
    return EXIT_SUCCESS;
}
