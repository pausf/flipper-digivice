#pragma once

// Cities (v3.3): names, exclusive monsters and the spawn roll. Pure C: no furi includes.

#include "digivice_expedition.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigiviceCityNone,
    DigiviceCityBarcelona,
    DigiviceCityMadrid,
    DigiviceCityValencia,
    DigiviceCitySevilla,
    DigiviceCityBilbao,
    DigiviceCityCount,
} DigiviceCity;

/** Proper noun; empty for DigiviceCityNone or out of range. */
const char* digivice_city_name(uint8_t city);
/** Monster of the city with index `idx` = city - 1 (0..DIGIVICE_CITY_COUNT-1). */
const char* digivice_city_monster_name(uint8_t idx);
DigiviceAttr digivice_city_monster_attr(uint8_t idx);
/** Champion-tier trail power at `area`. */
uint16_t digivice_city_monster_power(uint8_t area);
bool digivice_city_code(uint8_t code);
/** Rolls the city monster for a spawning trail encounter: its enc_foe code or
 * DIGIVICE_ENC_NONE. Needs a city and a Rookie or above. Never advances s->rng. */
uint8_t digivice_city_pick(const DigiviceState* s);

#ifdef __cplusplus
}
#endif
