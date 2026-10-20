// Host-side test for the homage partner rules. Run: applications/services/digivice/test/run.sh
#include "../digivice_homage.h"
#include "../digivice_config.h"
#include "../digivice_expedition.h"

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

#define T0 (10UL * 86400UL + 8UL * 3600UL)

static DigiviceState rookie(void) {
    DigiviceState s;
    digivice_rules_init(&s, T0);
    s.form = DigiviceFormKorno;
    s.prev_form = DigiviceFormKorno;
    return s;
}

#if DIGIVICE_EASTER_EGGS

static void test_enabled_and_tables(void) {
    CHECK(digivice_homage_enabled());
    for(uint8_t id = 0; id < DIGIVICE_HOMAGE_COUNT; id++) {
        CHECK(digivice_homage_name(id)[0] != '\0' && digivice_homage_name(id)[0] != '?');
        CHECK(digivice_homage_attr(id) < DigiviceAttrNulo);
        CHECK(digivice_homage_code(DIGIVICE_ENC_HOMAGE + id));
    }
    CHECK(digivice_homage_name(DIGIVICE_HOMAGE_COUNT)[0] == '?');
    CHECK(!digivice_homage_code(DIGIVICE_ENC_HOMAGE - 1));
    CHECK(!digivice_homage_code(DIGIVICE_ENC_HOMAGE + DIGIVICE_HOMAGE_COUNT));
    CHECK(!digivice_homage_code(DIGIVICE_ENC_NONE));
}

static void test_odds_table(void) {
    CHECK(digivice_homage_odds(0) == 0);
    CHECK(digivice_homage_odds(1) == 10);
    CHECK(digivice_homage_odds(2) == 20);
    CHECK(digivice_homage_odds(3) == 40);
    CHECK(digivice_homage_odds(4) == 100);
    CHECK(digivice_homage_odds(200) == 100);
}

static void test_pick_never_at_zero_always_at_four(void) {
    DigiviceState s = rookie();
    for(uint32_t i = 1; i <= 500; i++) {
        s.rng = (uint32_t)(i * 2654435761UL) | 1U;
        s.special_count = 0;
        CHECK(digivice_homage_pick(&s) == DIGIVICE_ENC_NONE);
        CHECK(s.special_count == 0);
        s.special_count = 4;
        uint8_t code = digivice_homage_pick(&s);
        CHECK(digivice_homage_code(code));
        CHECK(s.special_count == 0);
    }
}

static void test_pick_does_not_advance_rng(void) {
    DigiviceState s = rookie();
    s.special_count = 4;
    uint32_t rng = s.rng;
    digivice_homage_pick(&s);
    CHECK(s.rng == rng);
}

static void test_pick_rate_at_two(void) {
    DigiviceState s = rookie();
    uint32_t hits = 0;
    for(uint32_t i = 1; i <= 2000; i++) {
        s.rng = (uint32_t)(i * 2654435761UL) | 1U;
        s.special_count = 2;
        if(digivice_homage_pick(&s) != DIGIVICE_ENC_NONE) hits++;
    }
    CHECK(hits > 300 && hits < 500); // 20%
}

static void test_pick_needs_rookie(void) {
    DigiviceState s = rookie();
    s.form = DigiviceFormOrejin;
    s.prev_form = DigiviceFormOrejin;
    s.special_count = 4;
    CHECK(digivice_homage_pick(&s) == DIGIVICE_ENC_NONE);
    CHECK(s.special_count == 4);
}

static void test_pick_prefers_uncaught(void) {
    DigiviceState s = rookie();
    s.caught = (uint16_t)(DIGIVICE_HOMAGE_MASK & ~(1U << 5));
    for(uint32_t i = 1; i <= 200; i++) {
        s.rng = (uint32_t)(i * 2654435761UL) | 1U;
        s.special_count = 4;
        CHECK(digivice_homage_pick(&s) == DIGIVICE_ENC_HOMAGE + 5);
    }
    s.caught = DIGIVICE_HOMAGE_MASK;
    uint16_t seen = 0;
    for(uint32_t i = 1; i <= 400; i++) {
        s.rng = (uint32_t)(i * 2654435761UL) | 1U;
        s.special_count = 4;
        uint8_t code = digivice_homage_pick(&s);
        CHECK(digivice_homage_code(code));
        seen |= (uint16_t)(1U << (code - DIGIVICE_ENC_HOMAGE));
    }
    CHECK(seen == DIGIVICE_HOMAGE_MASK);
}

static void test_count_battle_saturates(void) {
    DigiviceState s = rookie();
    for(int i = 0; i < 10; i++) digivice_homage_count_battle(&s);
    CHECK(s.special_count == DIGIVICE_HOMAGE_GUARANTEE);
}

static void test_capture_pct(void) {
    DigiviceState s = rookie();
    s.mood = 0;
    CHECK(digivice_homage_capture_pct(&s) == 60);
    s.mood = 4;
    CHECK(digivice_homage_capture_pct(&s) == 80);
}

static void test_resolve(void) {
    DigiviceState s = rookie();
    s.mood = 4;
    s.special_count = 0;
    CHECK(digivice_homage_resolve(&s, 3, 79) == DigiviceCaptureCaught);
    CHECK(s.caught == (1U << 3) && s.companion == 3);
    CHECK(digivice_homage_resolve(&s, 3, 99) == DigiviceCaptureFriend);
    CHECK(s.caught == (1U << 3));
    CHECK(digivice_homage_resolve(&s, 6, 80) == DigiviceCaptureEscaped);
    CHECK(!(s.caught & (1U << 6)) && s.special_count == DIGIVICE_HOMAGE_RETRY);
    CHECK(digivice_homage_resolve(&s, 6, 0) == DigiviceCaptureCaught);
    CHECK(s.companion == 3); // the first one stays the companion
}

static void test_spawn_hook_in_the_walk(void) {
    DigiviceState s = rookie();
    s.distance = 100;
    s.paces_since_fight = DIGIVICE_ENCOUNTER_GUARANTEE;
    s.special_count = 4;
    digivice_expedition_walk(&s, DIGIVICE_PACE_S, T0 + DIGIVICE_PACE_S);
    CHECK(digivice_homage_code(s.enc_foe));
    CHECK(s.special_count == 0);
    CHECK(s.enc_expires == T0 + DIGIVICE_PACE_S + DIGIVICE_ENCOUNTER_TTL_S);
}

static void test_expired_partner_restores_pity(void) {
    DigiviceState s = rookie();
    s.enc_foe = DIGIVICE_ENC_HOMAGE + 2;
    s.enc_expires = T0 + 60;
    s.special_count = 0;
    digivice_expedition_expire(&s, T0 + 59);
    CHECK(s.enc_foe == DIGIVICE_ENC_HOMAGE + 2 && s.special_count == 0);
    digivice_expedition_expire(&s, T0 + 60);
    CHECK(s.enc_foe == DIGIVICE_ENC_NONE && s.special_count == DIGIVICE_HOMAGE_RETRY);
}

#else

static void test_everything_is_inert(void) {
    CHECK(!digivice_homage_enabled());
    CHECK(digivice_homage_name(0)[0] == '?');
    CHECK(!digivice_homage_code(DIGIVICE_ENC_HOMAGE));
    DigiviceState s = rookie();
    s.special_count = 4;
    CHECK(digivice_homage_pick(&s) == DIGIVICE_ENC_NONE);
    CHECK(digivice_homage_resolve(&s, 1, 0) == DigiviceCaptureNone);
    CHECK(s.caught == 0);
    s.enc_foe = DIGIVICE_ENC_HOMAGE;
    digivice_rules_sanitize(&s);
    CHECK(s.enc_foe == DIGIVICE_ENC_NONE);
}

#endif

static void test_hash_is_pure(void) {
    CHECK(digivice_rules_hash(5, 1) == digivice_rules_hash(5, 1));
    CHECK(digivice_rules_hash(5, 1) != digivice_rules_hash(5, 2));
    CHECK(digivice_rules_hash(5, 1) != digivice_rules_hash(6, 1));
}

int main(void) {
    test_hash_is_pure();
#if DIGIVICE_EASTER_EGGS
    test_enabled_and_tables();
    test_odds_table();
    test_pick_never_at_zero_always_at_four();
    test_pick_does_not_advance_rng();
    test_pick_rate_at_two();
    test_pick_needs_rookie();
    test_pick_prefers_uncaught();
    test_count_battle_saturates();
    test_capture_pct();
    test_resolve();
    test_spawn_hook_in_the_walk();
    test_expired_partner_restores_pity();
#else
    test_everything_is_inert();
#endif
    if(failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("digivice homage tests passed\n");
    return 0;
}
