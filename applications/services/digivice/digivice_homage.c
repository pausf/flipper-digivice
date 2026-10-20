#include "digivice_homage.h"
#include "digivice_config.h"

#if DIGIVICE_EASTER_EGGS

static const char* const names[DIGIVICE_HOMAGE_COUNT] = {
    "AGUMON",
    "GABUMON",
    "BIYOMON",
    "TENTOMON",
    "PALMON",
    "GOMAMON",
    "PATAMON",
    "GATOMON",
    "TERRIERMON",
};

static const uint8_t attrs[DIGIVICE_HOMAGE_COUNT] = {
    DigiviceAttrPortador,
    DigiviceAttrCifrado,
    DigiviceAttrRuido,
    DigiviceAttrRuido,
    DigiviceAttrCifrado,
    DigiviceAttrPortador,
    DigiviceAttrPortador,
    DigiviceAttrCifrado,
    DigiviceAttrRuido,
};

static const uint8_t odds[DIGIVICE_HOMAGE_GUARANTEE + 1] = DIGIVICE_HOMAGE_ODDS;

bool digivice_homage_enabled(void) {
    return true;
}

bool digivice_homage_code(uint8_t code) {
    return code >= DIGIVICE_ENC_HOMAGE && code < DIGIVICE_ENC_HOMAGE + DIGIVICE_HOMAGE_COUNT;
}

const char* digivice_homage_name(uint8_t id) {
    return id < DIGIVICE_HOMAGE_COUNT ? names[id] : "?";
}

DigiviceAttr digivice_homage_attr(uint8_t id) {
    return id < DIGIVICE_HOMAGE_COUNT ? (DigiviceAttr)attrs[id] : DigiviceAttrNulo;
}

uint8_t digivice_homage_odds(uint8_t count) {
    return odds[count < DIGIVICE_HOMAGE_GUARANTEE ? count : DIGIVICE_HOMAGE_GUARANTEE];
}

uint8_t digivice_homage_pick(DigiviceState* s) {
    if(digivice_rules_stage(s) < DIGIVICE_SPECIAL_MIN_STAGE) return DIGIVICE_ENC_NONE;
    uint32_t r = digivice_rules_hash(s->rng, 0xA6E5);
    if(r % 100 >= digivice_homage_odds(s->special_count)) return DIGIVICE_ENC_NONE;
    s->special_count = 0;
    uint16_t pool = (uint16_t)(~s->caught & DIGIVICE_HOMAGE_MASK);
    if(!pool) pool = DIGIVICE_HOMAGE_MASK;
    uint32_t k = (r >> 8) % (uint32_t)__builtin_popcount(pool);
    for(uint8_t id = 0; id < DIGIVICE_HOMAGE_COUNT; id++) {
        if(!((pool >> id) & 1U)) continue;
        if(k == 0) return (uint8_t)(DIGIVICE_ENC_HOMAGE + id);
        k--;
    }
    return DIGIVICE_ENC_NONE;
}

void digivice_homage_count_battle(DigiviceState* s) {
    if(s->special_count < DIGIVICE_HOMAGE_GUARANTEE) s->special_count++;
}

uint8_t digivice_homage_capture_pct(const DigiviceState* s) {
    uint32_t pct = DIGIVICE_CAPTURE_BASE_PCT + DIGIVICE_CAPTURE_MOOD_PCT * (uint32_t)s->mood;
    return (uint8_t)(pct < DIGIVICE_CAPTURE_MAX_PCT ? pct : DIGIVICE_CAPTURE_MAX_PCT);
}

DigiviceCapture digivice_homage_resolve(DigiviceState* s, uint8_t id, uint8_t roll) {
    if(id >= DIGIVICE_HOMAGE_COUNT) return DigiviceCaptureNone;
    if(s->caught & (1U << id)) return DigiviceCaptureFriend;
    if(roll < digivice_homage_capture_pct(s)) {
        s->caught |= (uint16_t)(1U << id);
        if(s->companion == DIGIVICE_COMPANION_NONE) s->companion = id;
        return DigiviceCaptureCaught;
    }
    s->special_count = DIGIVICE_HOMAGE_RETRY;
    return DigiviceCaptureEscaped;
}

#else

bool digivice_homage_enabled(void) {
    return false;
}

bool digivice_homage_code(uint8_t code) {
    (void)code;
    return false;
}

const char* digivice_homage_name(uint8_t id) {
    (void)id;
    return "?";
}

DigiviceAttr digivice_homage_attr(uint8_t id) {
    (void)id;
    return DigiviceAttrNulo;
}

uint8_t digivice_homage_odds(uint8_t count) {
    (void)count;
    return 0;
}

uint8_t digivice_homage_pick(DigiviceState* s) {
    (void)s;
    return DIGIVICE_ENC_NONE;
}

void digivice_homage_count_battle(DigiviceState* s) {
    (void)s;
}

uint8_t digivice_homage_capture_pct(const DigiviceState* s) {
    (void)s;
    return 0;
}

DigiviceCapture digivice_homage_resolve(DigiviceState* s, uint8_t id, uint8_t roll) {
    (void)s;
    (void)id;
    (void)roll;
    return DigiviceCaptureNone;
}

#endif
