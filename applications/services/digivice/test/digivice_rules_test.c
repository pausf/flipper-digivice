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
    CHECK(digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(s.hunger == 3);
    CHECK(s.corruption == 7);
    s.hunger = DIGIVICE_HUNGER_MAX;
    CHECK(!digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(s.corruption == 7);
    s.hunger = 0;
    s.corruption = 3;
    digivice_rules_feed(&s, DigiviceFoodMeat, T0);
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
    CHECK(digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(s.clean_since == T0);
    s.corruption = 0;
    s.hunger = 0;
    s.last_tick = T0 + H(12);
    digivice_rules_tick(&s, T0 + H(13));
    CHECK(s.corruption == 5 && s.clean_since == 0);

    // Clean again at T1, 14 h after T0: the 24 h clock must restart there.
    uint32_t t1 = T0 + H(14);
    CHECK(digivice_rules_feed(&s, DigiviceFoodMeat, t1));
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
    // The v1 file is 56 bytes, v2 appends 28 and v3 another 32, each keeping the older offsets.
    CHECK(sizeof(DigiviceStateV1) == 56);
    CHECK(sizeof(DigiviceStateV2) == 84);
    CHECK(sizeof(DigiviceState) == 140);
    CHECK(offsetof(DigiviceState, corrupt_acc_s) == offsetof(DigiviceStateV1, corrupt_acc_s));
    CHECK(offsetof(DigiviceState, light_on_asleep_s) ==
          offsetof(DigiviceStateV2, light_on_asleep_s));
    CHECK(offsetof(DigiviceState, area) == sizeof(DigiviceStateV2));
}

static void check_v3_defaults(const DigiviceState* s) {
    CHECK(s->area == 0 && s->cleared == 0 && s->enc_foe == DIGIVICE_ENC_NONE);
    CHECK(s->mid_state == 0 && s->item == DigiviceItemNone && s->reserved3 == 0);
    CHECK(s->hike_count == 0 && s->scan_count == 0 && s->paces_since_fight == 0);
    CHECK(s->distance == 480 && s->distance == digivice_rules_area_length(0));
    CHECK(s->pace_acc_s == 0 && s->enc_expires == 0 && s->bonus_window_start == 0);
    CHECK(s->injured_until == 0);
    CHECK(s->rng == ((s->last_tick ^ s->born_at ^ 0x9E3779B9UL) | 1UL));
    CHECK(s->rng != 0);
}

static void test_v3_defaults(void) {
    DigiviceState s = fresh(T0);
    check_v3_defaults(&s);
    s.area = 3;
    s.cleared = 7;
    s.item = DigiviceItemShield;
    digivice_rules_reset(&s, T0 + 10);
    check_v3_defaults(&s);
}

static void test_area_lengths(void) {
    const uint16_t lengths[DIGIVICE_AREA_COUNT] = DIGIVICE_AREA_LENGTHS;
    uint32_t total = 0;
    for(uint8_t a = 0; a < DIGIVICE_AREA_COUNT; a++) {
        CHECK(digivice_rules_area_length(a) == lengths[a]);
        total += lengths[a];
    }
    CHECK(total == 7320);
    CHECK(digivice_rules_area_length(0) == 480 && digivice_rules_area_length(7) == 1440);
    CHECK(digivice_rules_area_length(200) == 1440);
}

static DigiviceStateV2 v2_fixture(void) {
    DigiviceStateV2 v2;
    memset(&v2, 0, sizeof(v2));
    v2.form = DigiviceFormTagdrake;
    v2.prev_form = DigiviceFormBytemon;
    v2.evo_from = DigiviceFormNone;
    v2.hunger = 2;
    v2.mood = 3;
    v2.corruption = 15;
    v2.virux_type = DigiviceTypeSubGhz;
    v2.strength = 17;
    v2.wins = 4;
    v2.losses = 5;
    v2.data[0] = 31;
    v2.data[1] = 32;
    v2.data[2] = 33;
    v2.data[3] = 34;
    v2.exp = 777;
    v2.virux_expires = T0 + 900;
    v2.last_tick = T0 - 60;
    v2.born_at = T0 - DAY(9);
    v2.clean_since = 0;
    v2.hunger_acc_s = 101;
    v2.mood_acc_s = 102;
    v2.corrupt_acc_s = 103;
    v2.lang = DigiviceLangEn;
    v2.mode = DigiviceModeHacker;
    v2.poop = 2;
    v2.sick = 1;
    v2.care_mistakes = 3;
    v2.light_on = 0;
    v2.doses_needed = 2;
    v2.poop_acc_s = 104;
    v2.poop_dirty_since = T0 - H(2);
    v2.starving_since = T0 - H(3);
    v2.sick_since = T0 - H(4);
    v2.light_on_asleep_s = 105;
    return v2;
}

static void check_v2_prefix(const DigiviceState* s, const DigiviceStateV2* v2) {
    CHECK(s->form == v2->form && s->prev_form == v2->prev_form && s->evo_from == v2->evo_from);
    CHECK(s->hunger == v2->hunger && s->mood == v2->mood && s->corruption == v2->corruption);
    CHECK(s->virux_type == v2->virux_type && s->virux_expires == v2->virux_expires);
    CHECK(s->strength == v2->strength && s->wins == v2->wins && s->losses == v2->losses);
    CHECK(memcmp(s->data, v2->data, sizeof(s->data)) == 0);
    CHECK(s->exp == v2->exp && s->last_tick == v2->last_tick && s->born_at == v2->born_at);
    CHECK(s->clean_since == v2->clean_since && s->hunger_acc_s == v2->hunger_acc_s);
    CHECK(s->mood_acc_s == v2->mood_acc_s && s->corrupt_acc_s == v2->corrupt_acc_s);
    CHECK(s->lang == v2->lang && s->mode == v2->mode && s->poop == v2->poop);
    CHECK(s->sick == v2->sick && s->care_mistakes == v2->care_mistakes);
    CHECK(s->light_on == v2->light_on && s->doses_needed == v2->doses_needed);
    CHECK(s->poop_acc_s == v2->poop_acc_s && s->poop_dirty_since == v2->poop_dirty_since);
    CHECK(s->starving_since == v2->starving_since && s->sick_since == v2->sick_since);
    CHECK(s->light_on_asleep_s == v2->light_on_asleep_s);
}

static void test_migrate_v2_field_by_field(void) {
    DigiviceStateV2 v2 = v2_fixture();
    DigiviceState s;
    memset(&s, 0xAA, sizeof(s));
    digivice_rules_migrate_v2(&v2, &s);
    check_v2_prefix(&s, &v2);
    check_v3_defaults(&s);
}

static void test_migrate_chain_v1_to_v3(void) {
    DigiviceStateV1 v1 = {
        .form = DigiviceFormBeamdrake,
        .prev_form = DigiviceFormBytemon,
        .evo_from = DigiviceFormNone,
        .hunger = 1,
        .mood = 4,
        .corruption = 0,
        .virux_type = DigiviceTypeNone,
        .strength = 9,
        .wins = 2,
        .losses = 1,
        .data = {1, 2, 30, 4},
        .exp = 400,
        .virux_expires = 0,
        .last_tick = T0,
        .born_at = T0 - DAY(8),
        .clean_since = T0 - DAY(1),
    };
    DigiviceStateV2 v2;
    DigiviceState s;
    digivice_rules_migrate_v1(&v1, &v2);
    digivice_rules_migrate_v2(&v2, &s);
    CHECK(s.form == DigiviceFormBeamdrake && s.exp == 400 && s.strength == 9);
    CHECK(s.data[DigiviceTypeIr] == 30 && s.wins == 2 && s.losses == 1);
    CHECK(s.last_tick == T0 && s.born_at == T0 - DAY(8));
    CHECK(s.lang == DigiviceLangEs && s.mode == DigiviceModeNormal && s.light_on == 1);
    check_v3_defaults(&s);
}

static void test_sanitize_clamps_v3(void) {
    DigiviceState s = fresh(T0);
    s.area = 9;
    s.enc_foe = 2;
    s.mid_state = 7;
    s.item = 5;
    s.distance = 60000;
    s.hike_count = 200;
    s.scan_count = 200;
    s.paces_since_fight = 999;
    s.rng = 0;
    digivice_rules_sanitize(&s);
    CHECK(s.area == 0 && s.enc_foe == DIGIVICE_ENC_NONE && s.mid_state == 2);
    CHECK(s.item == DigiviceItemNone && s.distance == 480);
    CHECK(s.hike_count == DIGIVICE_HIKE_CAP && s.scan_count == DIGIVICE_SCAN_CAP);
    CHECK(s.paces_since_fight == DIGIVICE_ENCOUNTER_GUARANTEE && s.rng != 0);

    // A distance inside the area length is kept; each in-range pool index is kept.
    s.area = 7;
    s.distance = 1440;
    s.enc_foe = 1;
    s.enc_expires = T0 + 60;
    digivice_rules_sanitize(&s);
    CHECK(s.distance == 1440 && s.enc_foe == 1 && s.enc_expires == T0 + 60);

    // Both encounter slots set: the trail one goes.
    s.virux_type = DigiviceTypeNfc;
    s.virux_expires = T0 + 60;
    digivice_rules_sanitize(&s);
    CHECK(s.enc_foe == DIGIVICE_ENC_NONE && s.enc_expires == 0);
    CHECK(s.virux_type == DigiviceTypeNfc);
}

static void test_same_visible_ignores_pace_bookkeeping(void) {
    DigiviceState a = fresh(T0);
    DigiviceState b = a;
    b.pace_acc_s = 59;
    b.paces_since_fight = 12;
    b.rng ^= 0x1234;
    b.hike_count = 10;
    b.scan_count = 3;
    b.bonus_window_start = T0;
    b.distance = 300;
    CHECK(digivice_rules_same_visible(&a, &b));
    b.distance = 0;
    CHECK(!digivice_rules_same_visible(&a, &b));
    b = a;
    b.enc_foe = 1;
    CHECK(!digivice_rules_same_visible(&a, &b));
    b = a;
    b.mid_state = 1;
    CHECK(!digivice_rules_same_visible(&a, &b));
    b = a;
    b.item = DigiviceItemShield;
    CHECK(!digivice_rules_same_visible(&a, &b));
    b = a;
    b.injured_until = T0 + 3600;
    CHECK(!digivice_rules_same_visible(&a, &b));
}

static void test_rng(void) {
    DigiviceState a = fresh(T0);
    DigiviceState b = a;
    for(int i = 0; i < 100; i++) {
        uint32_t x = digivice_rules_rng(&a);
        CHECK(x != 0 && x == a.rng);
        CHECK(digivice_rules_rng_pct(&b) == x % 100);
    }
    a.rng = 0;
    CHECK(digivice_rules_rng(&a) != 0);
    a.rng = 0x55;
    digivice_rules_mix_entropy(&a, 0x55);
    CHECK(a.rng == 1);
    digivice_rules_mix_entropy(&a, 0xF0);
    CHECK(a.rng == 0xF1);
}

static void test_attr_triangle(void) {
    CHECK(digivice_rules_attr_adv(DigiviceAttrPortador, DigiviceAttrRuido) == 1);
    CHECK(digivice_rules_attr_adv(DigiviceAttrRuido, DigiviceAttrCifrado) == 1);
    CHECK(digivice_rules_attr_adv(DigiviceAttrCifrado, DigiviceAttrPortador) == 1);
    CHECK(digivice_rules_attr_adv(DigiviceAttrRuido, DigiviceAttrPortador) == -1);
    CHECK(digivice_rules_attr_adv(DigiviceAttrCifrado, DigiviceAttrRuido) == -1);
    CHECK(digivice_rules_attr_adv(DigiviceAttrPortador, DigiviceAttrCifrado) == -1);
    for(uint8_t a = 0; a < DigiviceAttrCount; a++) {
        CHECK(digivice_rules_attr_adv(a, a) == 0);
        CHECK(digivice_rules_attr_adv(a, DigiviceAttrNulo) == 0);
        CHECK(digivice_rules_attr_adv(DigiviceAttrNulo, a) == 0);
        CHECK(digivice_rules_attr_adv(a, 9) == 0 && digivice_rules_attr_adv(9, a) == 0);
    }
}

static void test_form_and_type_attr(void) {
    static const uint8_t expected[DigiviceFormVirubyte] = {
        [DigiviceFormByteegg] = DigiviceAttrNulo,
        [DigiviceFormBitling] = DigiviceAttrNulo,
        [DigiviceFormBytemon] = DigiviceAttrNulo,
        [DigiviceFormTagdrake] = DigiviceAttrCifrado,
        [DigiviceFormWavedrake] = DigiviceAttrPortador,
        [DigiviceFormBeamdrake] = DigiviceAttrRuido,
        [DigiviceFormPlugdrake] = DigiviceAttrNulo,
    };
    DigiviceState s = fresh(T0);
    for(uint8_t f = 0; f < DigiviceFormVirubyte; f++) {
        s.form = f;
        CHECK(digivice_rules_form_attr(&s) == expected[f]);
    }
    // Virubyte is static whatever it was before.
    s.form = DigiviceFormVirubyte;
    for(uint8_t f = 0; f < DigiviceFormVirubyte; f++) {
        s.prev_form = f;
        CHECK(digivice_rules_form_attr(&s) == DigiviceAttrRuido);
    }
    CHECK(digivice_rules_type_attr(DigiviceTypeNfc) == DigiviceAttrCifrado);
    CHECK(digivice_rules_type_attr(DigiviceTypeSubGhz) == DigiviceAttrPortador);
    CHECK(digivice_rules_type_attr(DigiviceTypeIr) == DigiviceAttrRuido);
    CHECK(digivice_rules_type_attr(DigiviceTypeUsb) == DigiviceAttrNulo);
    CHECK(digivice_rules_type_attr(DigiviceTypeNone) == DigiviceAttrNulo);
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
    DigiviceStateV2 s;
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
    CHECK(digivice_rules_feed(&s, DigiviceFoodMeat, T0 + 120));
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

static void test_bedtime_fires_once_when_the_window_opens(void) {
    DigiviceState s = baby(T0);
    uint32_t bed = DAY(10) + H(23);
    CHECK(digivice_rules_bedtime_started(&s, bed - 60, bed));
    CHECK(!digivice_rules_bedtime_started(&s, bed, bed + 60));
    CHECK(!digivice_rules_bedtime_started(&s, bed - 120, bed - 60));
    // A catch-up after the Flipper was off is not a bedtime.
    CHECK(!digivice_rules_bedtime_started(&s, bed - H(6), bed + H(2)));
    s.light_on = 0;
    CHECK(!digivice_rules_bedtime_started(&s, bed - 60, bed));
    DigiviceState egg = fresh(T0);
    CHECK(!digivice_rules_bedtime_started(&egg, bed - 60, bed));
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

static void test_v4_layout(void) {
    CHECK(sizeof(DigiviceStateV3) == 116);
    CHECK(sizeof(DigiviceState) == 140);
    CHECK(offsetof(DigiviceState, vigor) == sizeof(DigiviceStateV3));
    CHECK(offsetof(DigiviceState, rng) == offsetof(DigiviceStateV3, rng));
}

static void check_v4_defaults(const DigiviceState* s, uint32_t stage_since) {
    CHECK(s->vigor == DIGIVICE_VIGOR_START);
    CHECK(s->call_flags == 0);
    CHECK(s->feed_flags == (DIGIVICE_FEED_ARM_HUNGER | DIGIVICE_FEED_ARM_VIGOR));
    CHECK(s->evo_mistakes == 0 && s->train_count == 0 && s->battles == 0);
    CHECK(s->wins_stage == 0 && s->overfeed == 0 && s->night_owl == 0);
    CHECK(s->call_hunger_s == 0 && s->call_vigor_s == 0 && s->vigor_acc_s == 0);
    CHECK(s->stage_since == stage_since);
}

static void test_v4_defaults_on_init_and_reset(void) {
    DigiviceState s = fresh(T0);
    check_v4_defaults(&s, T0);
    s.vigor = 0;
    s.overfeed = 9;
    digivice_rules_reset(&s, T0 + 50);
    check_v4_defaults(&s, T0 + 50);
}

static void test_migrate_v3_keeps_everything(void) {
    DigiviceState src = fresh(T0);
    src.form = DigiviceFormTagdrake;
    src.prev_form = DigiviceFormBytemon;
    src.area = 3;
    src.cleared = 5;
    src.rng = 0xDEADBEEFUL;
    src.hunger = 2;
    src.lang = DigiviceLangEn;
    DigiviceStateV3 v3;
    memcpy(&v3, &src, sizeof(v3));
    DigiviceState out;
    memset(&out, 0xA5, sizeof(out));
    digivice_rules_migrate_v3(&v3, &out);
    CHECK(memcmp(&out, &v3, sizeof(v3)) == 0);
    check_v4_defaults(&out, v3.last_tick);
}

static void test_migrate_v2_applies_v4_defaults(void) {
    DigiviceStateV2 v2 = v2_fixture();
    DigiviceState out;
    digivice_rules_migrate_v2(&v2, &out);
    check_v4_defaults(&out, v2.last_tick);
}

static void test_sanitize_v4_fields(void) {
    DigiviceState s = fresh(T0);
    s.vigor = 9;
    s.call_flags = 0xFF;
    s.feed_flags = 0xFF;
    s.call_hunger_s = 60000;
    s.call_vigor_s = 60000;
    s.vigor_acc_s = DIGIVICE_VIGOR_DECAY_S + 5;
    digivice_rules_sanitize(&s);
    CHECK(s.vigor == DIGIVICE_VIGOR_MAX);
    CHECK(s.call_flags == (DIGIVICE_CALL_HUNGER | DIGIVICE_CALL_VIGOR));
    CHECK(s.feed_flags == (DIGIVICE_FEED_ARM_HUNGER | DIGIVICE_FEED_ARM_VIGOR));
    CHECK(s.call_hunger_s == DIGIVICE_CALL_WINDOW_S && s.call_vigor_s == DIGIVICE_CALL_WINDOW_S);
    CHECK(s.vigor_acc_s == 0);
}

static void test_meat_fills_hunger_and_refuses_when_full(void) {
    DigiviceState s = fresh(T0);
    s.hunger = 3;
    CHECK(digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(s.hunger == DIGIVICE_HUNGER_MAX && s.overfeed == 0);
    CHECK(!digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(s.hunger == DIGIVICE_HUNGER_MAX && s.overfeed == 1);
}

static void test_refusal_counts_once_until_the_meter_drains(void) {
    DigiviceState s = fresh(T0);
    s.hunger = DIGIVICE_HUNGER_MAX;
    CHECK(!digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(!digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(!digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(s.overfeed == 1);
    // One heart drains (a training costs one), the next refusal counts again.
    CHECK(digivice_rules_train(&s, true, T0 + 3600) == DigiviceResultOk);
    CHECK(digivice_rules_feed(&s, DigiviceFoodMeat, T0 + 3600));
    CHECK(!digivice_rules_feed(&s, DigiviceFoodMeat, T0 + 3600));
    CHECK(s.overfeed == 2);
}

static void test_protein_fills_vigor_and_strength(void) {
    DigiviceState s = fresh(T0);
    s.vigor = 3;
    s.strength = 5;
    CHECK(digivice_rules_feed(&s, DigiviceFoodProtein, T0));
    CHECK(s.vigor == DIGIVICE_VIGOR_MAX && s.strength == 5 + DIGIVICE_PROTEIN_STRENGTH);
    CHECK(s.hunger == fresh(T0).hunger);
    CHECK(!digivice_rules_feed(&s, DigiviceFoodProtein, T0));
    CHECK(s.overfeed == 1 && s.strength == 5 + DIGIVICE_PROTEIN_STRENGTH);
}

static void test_each_food_has_its_own_arm_bit(void) {
    DigiviceState s = fresh(T0);
    s.hunger = DIGIVICE_HUNGER_MAX;
    s.vigor = DIGIVICE_VIGOR_MAX;
    CHECK(!digivice_rules_feed(&s, DigiviceFoodMeat, T0));
    CHECK(!digivice_rules_feed(&s, DigiviceFoodProtein, T0));
    CHECK(s.overfeed == 2);
}

static void test_vigor_decays_one_heart_per_four_awake_hours(void) {
    DigiviceState s = fresh(10 * 3600UL);
    s.vigor = 3;
    digivice_rules_tick(&s, 10 * 3600UL + DIGIVICE_VIGOR_DECAY_S);
    CHECK(s.vigor == 2);
    CHECK(s.feed_flags & DIGIVICE_FEED_ARM_VIGOR);
}

static void test_vigor_ticks_match_one_catch_up(void) {
    DigiviceState a = fresh(8 * 3600UL);
    DigiviceState b = a;
    a.vigor = b.vigor = 4;
    a.hunger = b.hunger = DIGIVICE_HUNGER_MAX;
    uint32_t end = 8 * 3600UL + 5 * 3600UL;
    for(uint32_t t = 8 * 3600UL + 60; t <= end; t += 60) digivice_rules_tick(&a, t);
    digivice_rules_tick(&b, end);
    CHECK(a.vigor == b.vigor && a.vigor_acc_s == b.vigor_acc_s);
}

static void test_stage_and_attr_come_from_the_roster(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormAriete;
    CHECK(digivice_rules_stage(&s) == DigiviceStageChampion);
    CHECK(digivice_rules_form_attr(&s) == DigiviceAttrPortador);
    s.form = DigiviceFormNularca;
    CHECK(digivice_rules_stage(&s) == DigiviceStageMega);
    s.form = DigiviceFormCharco;
    CHECK(digivice_rules_stage(&s) == DigiviceStageRookie);
    s.form = DigiviceFormVirubyte;
    s.prev_form = DigiviceFormLanzero;
    CHECK(digivice_rules_stage(&s) == DigiviceStageUltimate);
    CHECK(digivice_rules_form_attr(&s) == DigiviceAttrRuido);
}

static void test_training_counts_successes_only(void) {
    DigiviceState s = fresh(10 * 3600UL);
    s.hunger = DIGIVICE_HUNGER_MAX;
    CHECK(digivice_rules_train(&s, true, 10 * 3600UL) == DigiviceResultOk);
    CHECK(digivice_rules_train(&s, false, 10 * 3600UL) == DigiviceResultOk);
    CHECK(s.train_count == 1);
}

static void test_poop_mistake_counts_for_the_evolution_for_now(void) {
    DigiviceState s = fresh(8 * 3600UL);
    s.poop = 1;
    s.poop_dirty_since = 8 * 3600UL;
    digivice_rules_tick(&s, 8 * 3600UL + DIGIVICE_POOP_MISTAKE_S);
    CHECK(s.care_mistakes >= 1 && s.evo_mistakes >= 1);
}

static void test_night_owl_counts_the_light_left_on(void) {
    DigiviceState s = fresh(22 * 3600UL);
    s.form = DigiviceFormKorno;
    s.hunger = DIGIVICE_HUNGER_MAX;
    digivice_rules_tick(&s, 23 * 3600UL + 1800UL); // asleep from 23:00, light on
    CHECK(s.night_owl == 0);
    digivice_rules_tick(&s, 24 * 3600UL + 120UL);
    CHECK(s.night_owl == 1);
    digivice_rules_tick(&s, 24 * 3600UL + 5 * 3600UL);
    CHECK(s.night_owl == 1); // once per sleep window
}

static void test_count_battle_ignores_nothing_and_saturates(void) {
    DigiviceState s = fresh(T0);
    digivice_rules_count_battle(&s, true);
    digivice_rules_count_battle(&s, false);
    CHECK(s.battles == 2 && s.wins_stage == 1);
    s.battles = 255;
    s.wins_stage = 255;
    digivice_rules_count_battle(&s, true);
    CHECK(s.battles == 255 && s.wins_stage == 255);
}

static void test_egg_hatches_into_a_baby_of_either_line(void) {
    DigiviceState s = fresh(T0);
    CHECK(!digivice_rules_evolve(&s, T0 + DIGIVICE_HATCH_S - 1));
    CHECK(digivice_rules_evolve(&s, T0 + DIGIVICE_HATCH_S));
    CHECK(s.form == DigiviceFormPipo || s.form == DigiviceFormChispi);
    CHECK(s.evo_from == DigiviceFormByteegg);
    CHECK(s.stage_since == T0 + DIGIVICE_HATCH_S);
}

static void test_both_egg_lines_are_reachable(void) {
    bool seen_pipo = false;
    bool seen_chispi = false;
    for(uint32_t seed = 1; seed < 60; seed++) {
        DigiviceState s = fresh(T0);
        s.rng = seed * 2654435761UL | 1UL;
        digivice_rules_evolve(&s, T0 + DIGIVICE_HATCH_S);
        seen_pipo |= s.form == DigiviceFormPipo;
        seen_chispi |= s.form == DigiviceFormChispi;
    }
    CHECK(seen_pipo && seen_chispi);
}

static void test_evolution_resets_counters_and_keeps_lifetime_record(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormPipo;
    s.stage_since = T0;
    s.train_count = 9;
    s.battles = 4;
    s.wins_stage = 3;
    s.overfeed = 2;
    s.night_owl = 1;
    s.evo_mistakes = 5;
    s.wins = 40;
    s.losses = 12;
    CHECK(digivice_rules_evolve(&s, T0 + DIGIVICE_EVO_BABY1_S));
    CHECK(s.form == DigiviceFormOrejin);
    CHECK(s.train_count == 0 && s.battles == 0 && s.wins_stage == 0);
    CHECK(s.overfeed == 0 && s.night_owl == 0 && s.evo_mistakes == 0);
    CHECK(s.wins == 40 && s.losses == 12);
    CHECK(s.stage_since == T0 + DIGIVICE_EVO_BABY1_S);
}

static void test_a_pending_cutscene_blocks_the_next_evolution(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormPipo;
    s.stage_since = T0;
    CHECK(digivice_rules_evolve(&s, T0 + DIGIVICE_EVO_BABY1_S));
    uint32_t later = T0 + DIGIVICE_EVO_BABY1_S + DIGIVICE_EVO_BABY2_S;
    CHECK(!digivice_rules_evolve(&s, later));
    digivice_rules_ack_evolution(&s);
    CHECK(digivice_rules_evolve(&s, later));
}

static void test_corruption_transform_does_not_reset_the_stage(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormAriete;
    s.stage_since = T0 - 5000;
    s.train_count = 7;
    digivice_rules_add_corruption(&s, DIGIVICE_CORRUPTION_MAX, T0);
    CHECK(s.form == DigiviceFormVirubyte && s.prev_form == DigiviceFormAriete);
    CHECK(s.stage_since == T0 - 5000 && s.train_count == 7);
}

static void test_legacy_forms_move_onto_the_new_tree(void) {
    const uint8_t from[] = {
        DigiviceFormBitling,   DigiviceFormBytemon,   DigiviceFormTagdrake,
        DigiviceFormWavedrake, DigiviceFormBeamdrake, DigiviceFormPlugdrake,
    };
    const uint8_t to[] = {
        DigiviceFormPipo,   DigiviceFormKorno,  DigiviceFormCerrojo,
        DigiviceFormAriete, DigiviceFormFuzzcan, DigiviceFormChatarro,
    };
    for(size_t i = 0; i < sizeof(from); i++) {
        DigiviceState s = fresh(T0);
        s.form = from[i];
        s.wins = 17;
        CHECK(digivice_rules_upgrade_legacy(&s, T0 + 500));
        CHECK(s.form == to[i]);
        CHECK(s.stage_since == T0 + 500 && s.wins == 17);
        CHECK(!digivice_rules_upgrade_legacy(&s, T0 + 600));
        CHECK(s.stage_since == T0 + 500);
    }
}

static void test_legacy_upgrade_keeps_attributes_and_maps_hidden_slots(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormVirubyte;
    s.prev_form = DigiviceFormTagdrake;
    s.evo_from = DigiviceFormBytemon;
    CHECK(digivice_rules_upgrade_legacy(&s, T0 + 5));
    CHECK(s.form == DigiviceFormVirubyte);
    CHECK(s.prev_form == DigiviceFormCerrojo && s.evo_from == DigiviceFormKorno);
    s.form = DigiviceFormWavedrake;
    s.prev_form = DigiviceFormNone;
    digivice_rules_upgrade_legacy(&s, T0 + 6);
    CHECK(digivice_rules_form_attr(&s) == DigiviceAttrPortador);
}

static void test_player_type_follows_the_attribute(void) {
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormCerrojo;
    CHECK(digivice_rules_player_type(&s) == DigiviceTypeNfc);
    s.form = DigiviceFormAriete;
    CHECK(digivice_rules_player_type(&s) == DigiviceTypeSubGhz);
    s.form = DigiviceFormFuzzcan;
    CHECK(digivice_rules_player_type(&s) == DigiviceTypeIr);
    s.form = DigiviceFormChatarro;
    CHECK(digivice_rules_player_type(&s) == DigiviceTypeUsb);
}

// The real pet at the v3.2 update: a saved v3 BITLING, migrated to v4 (clock re-anchored to the
// load time, as the service does), then moved onto the tree.
static void test_the_neglected_bitling_survives_the_update(void) {
    uint32_t load = T0 + H(300);
    DigiviceState s = fresh(T0);
    s.form = DigiviceFormBitling;
    s.corruption = 100;
    s.sick = 1;
    s.sick_since = T0;
    s.doses_needed = 1;
    s.hunger = 0;
    s.mood = 0;
    s.exp = 12;
    s.last_tick = load;
    s.stage_since = load;
    CHECK(digivice_rules_upgrade_legacy(&s, load));
    CHECK(s.form == DigiviceFormPipo);
    CHECK(s.corruption == 100 && s.sick == 1 && s.hunger == 0 && s.mood == 0 && s.exp == 12);

    // First ticks: nothing evolves, nothing transforms, the stage timer runs from the load.
    digivice_rules_tick(&s, load + 60);
    CHECK(!digivice_rules_evolve(&s, load + 60));
    CHECK(s.form == DigiviceFormPipo && s.evo_from == DigiviceFormNone);
    CHECK(s.stage_since == load);
    CHECK(s.corruption == 100 && s.sick == 1 && s.hunger == 0 && s.mood == 0 && s.exp == 12);

    // A Baby never turns into a VIRUBYTE, however corrupted.
    digivice_rules_tick(&s, load + DIGIVICE_EVO_BABY1_S - 60);
    CHECK(s.form == DigiviceFormPipo);
    CHECK(!digivice_rules_evolve(&s, load + DIGIVICE_EVO_BABY1_S - 1));

    // Only when its own 10 minutes are up does it grow, along the tree.
    CHECK(digivice_rules_evolve(&s, load + DIGIVICE_EVO_BABY1_S));
    CHECK(s.form == DigiviceFormOrejin && s.evo_from == DigiviceFormPipo);
}

int main(void) {
    test_stage_and_attr_come_from_the_roster();
    test_egg_hatches_into_a_baby_of_either_line();
    test_both_egg_lines_are_reachable();
    test_evolution_resets_counters_and_keeps_lifetime_record();
    test_a_pending_cutscene_blocks_the_next_evolution();
    test_corruption_transform_does_not_reset_the_stage();
    test_legacy_forms_move_onto_the_new_tree();
    test_legacy_upgrade_keeps_attributes_and_maps_hidden_slots();
    test_player_type_follows_the_attribute();
    test_the_neglected_bitling_survives_the_update();
    test_training_counts_successes_only();
    test_poop_mistake_counts_for_the_evolution_for_now();
    test_night_owl_counts_the_light_left_on();
    test_count_battle_ignores_nothing_and_saturates();
    test_meat_fills_hunger_and_refuses_when_full();
    test_refusal_counts_once_until_the_meter_drains();
    test_protein_fills_vigor_and_strength();
    test_each_food_has_its_own_arm_bit();
    test_vigor_decays_one_heart_per_four_awake_hours();
    test_vigor_ticks_match_one_catch_up();
    test_init();
    test_v4_layout();
    test_v4_defaults_on_init_and_reset();
    test_migrate_v3_keeps_everything();
    test_migrate_v2_applies_v4_defaults();
    test_sanitize_v4_fields();
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
    test_champion_and_corrupt_do_not_evolve();
    test_deed_types();
    test_deed_grants_exp_and_data();
    test_spawn_rules();
    test_deed_out_of_range_is_ignored();
    test_spawn_deed_when_ineligible_still_rewards();
    test_v2_defaults_and_layout();
    test_v3_defaults();
    test_area_lengths();
    test_migrate_v2_field_by_field();
    test_migrate_chain_v1_to_v3();
    test_sanitize_clamps_v3();
    test_same_visible_ignores_pace_bookkeeping();
    test_rng();
    test_attr_triangle();
    test_form_and_type_attr();
    test_normal_mode_ignores_deeds();
    test_train_grants_exp_in_both_modes();
    test_train_refused_while_sick_or_asleep();
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
    test_bedtime_fires_once_when_the_window_opens();
    test_60s_ticks_match_one_tick_with_care();

    if(failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("digivice_rules: all checks passed\n");
    return EXIT_SUCCESS;
}
