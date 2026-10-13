// Host-side test for the pure digivice rules. Run: applications/services/digivice/test/run.sh
#include "../digivice_rules.h"
#include "../digivice_config.h"

#include <stdio.h>
#include <stdlib.h>

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

static void test_egg_hatches_after_1h(void) {
    DigiviceState s = fresh(T0);
    CHECK(!digivice_rules_evolve(&s, T0 + H(1) - 1));
    CHECK(s.form == DigiviceFormByteegg);
    CHECK(digivice_rules_evolve(&s, T0 + H(1)));
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
    DigiviceState s = fresh(T0);
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

    DigiviceState baby = fresh(T0);
    baby.form = DigiviceFormBitling;
    CHECK(!digivice_rules_on_deed(&baby, DolphinDeedNfcRead, 1, T0, 0));

    DigiviceState s = rookie(T0);
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

    DigiviceState c = rookie(T0);
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
    test_egg_hatches_after_1h();
    test_baby_to_rookie_at_50_exp();
    test_champion_paths_and_ties();
    test_champion_and_corrupt_do_not_evolve();
    test_deed_types();
    test_deed_grants_exp_and_data();
    test_spawn_rules();
    test_player_type_by_form();

    if(failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("digivice_rules: all checks passed\n");
    return EXIT_SUCCESS;
}
