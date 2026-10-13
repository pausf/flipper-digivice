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
    DigiviceState a = fresh(T0);
    DigiviceState b = fresh(T0);
    digivice_rules_tick(&a, T0 + H(48));
    digivice_rules_tick(&b, T0 + DAY(30));
    CHECK(a.hunger == b.hunger && a.mood == b.mood && a.corruption == b.corruption);
    CHECK(b.last_tick == T0 + DAY(30));
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
    s.last_tick = T0 + H(25);
    digivice_rules_tick(&s, T0 + H(25));
    CHECK(s.form == DigiviceFormVirubyte);
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

    if(failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("digivice_rules: all checks passed\n");
    return EXIT_SUCCESS;
}
