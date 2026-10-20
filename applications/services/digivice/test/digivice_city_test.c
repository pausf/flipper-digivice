// Host-side test for the city rules. Run: applications/services/digivice/test/run.sh
#include "../digivice_city.h"
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

static void test_names_and_monsters(void) {
    CHECK(digivice_city_name(DigiviceCityNone)[0] == '\0');
    CHECK(strcmp(digivice_city_name(DigiviceCityBarcelona), "BARCELONA") == 0);
    CHECK(strcmp(digivice_city_name(DigiviceCityBilbao), "BILBAO") == 0);
    CHECK(strcmp(digivice_city_monster_name(0), "TRENCADIX") == 0);
    CHECK(strcmp(digivice_city_monster_name(4), "ESCAMATI") == 0);
    CHECK(digivice_city_monster_attr(0) == DigiviceAttrCifrado);
    CHECK(digivice_city_monster_attr(1) == DigiviceAttrPortador);
    CHECK(digivice_city_monster_attr(2) == DigiviceAttrRuido);
    CHECK(digivice_city_monster_attr(3) == DigiviceAttrPortador);
    CHECK(digivice_city_monster_attr(4) == DigiviceAttrCifrado);
    CHECK(digivice_city_code(DIGIVICE_ENC_CITY));
    CHECK(digivice_city_code(DIGIVICE_ENC_CITY + DIGIVICE_CITY_COUNT - 1));
    CHECK(!digivice_city_code(DIGIVICE_ENC_CITY + DIGIVICE_CITY_COUNT));
    CHECK(!digivice_city_code(1));
}

static void test_monster_power_is_champion_tier(void) {
    for(uint8_t area = 0; area < DIGIVICE_AREA_COUNT; area++) {
        CHECK(
            digivice_city_monster_power(area) ==
            digivice_expedition_foe_power(area, DigiviceFoeFuzzcan));
    }
}

static void test_pick(void) {
    DigiviceState s = rookie();
    uint32_t rng = s.rng;
    CHECK(digivice_city_pick(&s) == DIGIVICE_ENC_NONE); // no city
    for(uint8_t city = 1; city <= DIGIVICE_CITY_COUNT; city++) {
        s.city = city;
        uint32_t hits = 0;
        for(uint32_t i = 1; i <= 1000; i++) {
            s.rng = (uint32_t)(i * 2654435761UL) | 1U;
            uint8_t code = digivice_city_pick(&s);
            if(code == DIGIVICE_ENC_NONE) continue;
            CHECK(code == DIGIVICE_ENC_CITY + city - 1);
            hits++;
        }
        CHECK(hits > 220 && hits < 380); // 30%
    }
    s.rng = rng;
    s.city = 2;
    digivice_city_pick(&s);
    CHECK(s.rng == rng); // never advances the rng
    s.form = DigiviceFormOrejin;
    s.prev_form = DigiviceFormOrejin;
    for(uint32_t i = 1; i <= 100; i++) {
        s.rng = (uint32_t)(i * 2654435761UL) | 1U;
        CHECK(digivice_city_pick(&s) == DIGIVICE_ENC_NONE); // Rookie or above only
    }
}

static void test_spawn_hook_picks_the_city_monster(void) {
    DigiviceState s = rookie();
    s.city = 3;
    uint32_t seen_city = 0;
    uint32_t seen_pool = 0;
    for(uint32_t i = 1; i <= 200; i++) {
        s.rng = (uint32_t)(i * 2654435761UL) | 1U;
        s.distance = 100;
        s.enc_foe = DIGIVICE_ENC_NONE;
        s.paces_since_fight = DIGIVICE_ENCOUNTER_GUARANTEE;
        digivice_expedition_walk(&s, DIGIVICE_PACE_S, T0 + DIGIVICE_PACE_S);
        if(s.enc_foe == DIGIVICE_ENC_CITY + 2) seen_city++;
        else if(s.enc_foe <= 1) seen_pool++;
    }
    CHECK(seen_city > 30 && seen_pool > 90 && seen_city + seen_pool == 200);
}

int main(void) {
    test_names_and_monsters();
    test_monster_power_is_champion_tier();
    test_pick();
    test_spawn_hook_picks_the_city_monster();

    if(failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("digivice_city: all checks passed\n");
    return EXIT_SUCCESS;
}
