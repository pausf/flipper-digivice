#include "digivice_city.h"
#include "digivice_config.h"

static const char* const city_names[DigiviceCityCount] = {
    "",
    "BARCELONA",
    "MADRID",
    "VALENCIA",
    "SEVILLA",
    "BILBAO",
};

static const char* const monster_names[DIGIVICE_CITY_COUNT] = {
    "TRENCADIX",
    "MADROSO",
    "TRACALUZ",
    "GIRALDIN",
    "ESCAMATI",
};

static const uint8_t monster_attrs[DIGIVICE_CITY_COUNT] = {
    DigiviceAttrCifrado,
    DigiviceAttrPortador,
    DigiviceAttrRuido,
    DigiviceAttrPortador,
    DigiviceAttrCifrado,
};

const char* digivice_city_name(uint8_t city) {
    return city < DigiviceCityCount ? city_names[city] : "";
}

const char* digivice_city_monster_name(uint8_t idx) {
    return idx < DIGIVICE_CITY_COUNT ? monster_names[idx] : "?";
}

DigiviceAttr digivice_city_monster_attr(uint8_t idx) {
    return idx < DIGIVICE_CITY_COUNT ? (DigiviceAttr)monster_attrs[idx] : DigiviceAttrNulo;
}

uint16_t digivice_city_monster_power(uint8_t area) {
    return digivice_expedition_foe_power(area, DigiviceFoeFuzzcan);
}

bool digivice_city_code(uint8_t code) {
    return code >= DIGIVICE_ENC_CITY && code < DIGIVICE_ENC_CITY + DIGIVICE_CITY_COUNT;
}

uint8_t digivice_city_pick(const DigiviceState* s) {
    if(s->city == 0 || s->city > DIGIVICE_CITY_COUNT) return DIGIVICE_ENC_NONE;
    if(digivice_rules_stage(s) < DIGIVICE_SPECIAL_MIN_STAGE) return DIGIVICE_ENC_NONE;
    if(digivice_rules_hash(s->rng, 0xC17E) % 100 >= DIGIVICE_CITY_PCT) return DIGIVICE_ENC_NONE;
    return (uint8_t)(DIGIVICE_ENC_CITY + s->city - 1);
}
