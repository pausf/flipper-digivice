#include "digivice_expedition.h"
#include "digivice_config.h"
#include "digivice_homage.h"

static const uint8_t pools[DIGIVICE_AREA_COUNT][2] = {
    {DigiviceFoeEstatik, DigiviceFoeKorno},
    {DigiviceFoeChipko, DigiviceFoeEstatik},
    {DigiviceFoeKorno, DigiviceFoeChipko},
    {DigiviceFoeMokobit, DigiviceFoeChipko},
    {DigiviceFoeFuzzcan, DigiviceFoeKorno},
    {DigiviceFoeVampixel, DigiviceFoeChipko},
    {DigiviceFoeSilbapo, DigiviceFoeMokobit},
    {DigiviceFoeChatarro, DigiviceFoeFuzzcan},
};

static const uint8_t foe_attrs[DigiviceFoeCount] = {
    [DigiviceFoeEstatik] = DigiviceAttrRuido,
    [DigiviceFoeChipko] = DigiviceAttrCifrado,
    [DigiviceFoeKorno] = DigiviceAttrPortador,
    [DigiviceFoeMokobit] = DigiviceAttrNulo,
    [DigiviceFoeFuzzcan] = DigiviceAttrRuido,
    [DigiviceFoeVampixel] = DigiviceAttrRuido,
    [DigiviceFoeSilbapo] = DigiviceAttrRuido,
    [DigiviceFoeChatarro] = DigiviceAttrNulo,
};

static const char* const foe_names[DigiviceFoeCount] = {
    "ESTATIK",
    "CHIPKO",
    "KORNO",
    "MOKOBIT",
    "FUZZCAN",
    "VAMPIXEL",
    "SILBAPO",
    "CHATARRO",
};

static const uint8_t boss_attrs[DIGIVICE_AREA_COUNT] = {
    DigiviceAttrRuido,
    DigiviceAttrCifrado,
    DigiviceAttrPortador,
    DigiviceAttrCifrado,
    DigiviceAttrRuido,
    DigiviceAttrCifrado,
    DigiviceAttrRuido,
    DigiviceAttrNulo,
};

static const char* const boss_names[DIGIVICE_AREA_COUNT] = {
    "CHISPAPO",
    "CABLEBOA",
    "MASTIRAM",
    "CRISTALOK",
    "GUSAFERRO",
    "CENTINELA",
    "DESBORDE",
    "NEGATIVO",
};

static const uint8_t midway_kinds[DIGIVICE_AREA_COUNT] = {
    DigiviceMidwaySpring,
    DigiviceMidwayCache,
    DigiviceMidwayShortcut,
    DigiviceMidwaySpring,
    DigiviceMidwayCache,
    DigiviceMidwayShortcut,
    DigiviceMidwaySpring,
    DigiviceMidwayCache,
};

static const uint8_t area_powers[DIGIVICE_AREA_COUNT] = DIGIVICE_AREA_POWER;

static uint8_t clamp_area(uint8_t area) {
    return area < DIGIVICE_AREA_COUNT ? area : DIGIVICE_AREA_COUNT - 1;
}

DigiviceFoe digivice_expedition_pool_foe(uint8_t area, uint8_t index) {
    return (DigiviceFoe)pools[clamp_area(area)][index & 1];
}

DigiviceAttr digivice_expedition_foe_attr(DigiviceFoe foe) {
    return foe < DigiviceFoeCount ? (DigiviceAttr)foe_attrs[foe] : DigiviceAttrNulo;
}

bool digivice_expedition_foe_champion(DigiviceFoe foe) {
    return foe >= DigiviceFoeFuzzcan && foe < DigiviceFoeCount;
}

const char* digivice_expedition_foe_name(DigiviceFoe foe) {
    return foe < DigiviceFoeCount ? foe_names[foe] : "?";
}

uint16_t digivice_expedition_foe_power(uint8_t area, DigiviceFoe foe) {
    uint16_t power = area_powers[clamp_area(area)];
    if(digivice_expedition_foe_champion(foe)) power = power * DIGIVICE_CHAMP_FOE_X10 / 10;
    return power;
}

uint16_t digivice_expedition_boss_power(uint8_t area) {
    return area_powers[clamp_area(area)] * DIGIVICE_BOSS_X10 / 10;
}

DigiviceAttr digivice_expedition_boss_attr(uint8_t area) {
    return (DigiviceAttr)boss_attrs[clamp_area(area)];
}

const char* digivice_expedition_boss_name(uint8_t area) {
    return boss_names[clamp_area(area)];
}

DigiviceMidwayKind digivice_expedition_midway_kind(uint8_t area) {
    return (DigiviceMidwayKind)midway_kinds[clamp_area(area)];
}

bool digivice_expedition_streak_area(uint8_t area) {
    return area >= DIGIVICE_STREAK_FIRST_AREA;
}

bool digivice_expedition_area_unlocked(const DigiviceState* s, uint8_t area) {
    if(area >= DIGIVICE_AREA_COUNT) return false;
    return area == 0 || (s->cleared & (1U << (area - 1)));
}

bool digivice_expedition_can_walk(const DigiviceState* s) {
    if(digivice_rules_stage(s) < DigiviceStageBaby || s->sick) return false;
    // A boss behind a closed gate keeps the trail alive: encounters go on while the pet grows.
    return s->distance > 0 ||
           (digivice_expedition_boss_ready(s) && !digivice_expedition_boss_gate_open(s));
}

bool digivice_expedition_trail_pending(const DigiviceState* s, uint32_t now) {
    return s->enc_foe != DIGIVICE_ENC_NONE && now < s->enc_expires;
}

void digivice_expedition_expire(DigiviceState* s, uint32_t now) {
    if(s->enc_foe != DIGIVICE_ENC_NONE && now >= s->enc_expires) {
        s->enc_foe = DIGIVICE_ENC_NONE;
        s->enc_expires = 0;
    }
}

// One pace at instant t: distance, midway, then the encounter roll. Returns true when this
// pace arrived at the boss. At distance 0 (closed gate) it is only the encounter roll.
static bool pace(DigiviceState* s, uint32_t t) {
    digivice_expedition_expire(s, t);
    if(s->paces_since_fight < DIGIVICE_ENCOUNTER_GUARANTEE) s->paces_since_fight++;
    if(s->distance > 0) {
        s->distance--;
        if(s->mid_state == 0 && s->distance <= digivice_rules_area_length(s->area) / 2) {
            s->mid_state = 1;
        }
        if(s->distance == 0) {
            // The boss is ready: no half-pace carried into the boss wait.
            s->pace_acc_s = 0;
            return true;
        }
    }
    if(s->enc_foe != DIGIVICE_ENC_NONE || digivice_rules_virux_active(s, t)) return false;
    bool spawn = s->paces_since_fight >= DIGIVICE_ENCOUNTER_GUARANTEE ||
                 digivice_rules_rng(s) % 100 < DIGIVICE_ENCOUNTER_PCT;
    if(!spawn) return false;
    uint8_t code = digivice_homage_pick(s);
    if(code == DIGIVICE_ENC_NONE) code = (uint8_t)(digivice_rules_rng(s) % 2);
    s->enc_foe = code;
    s->enc_expires = t + DIGIVICE_ENCOUNTER_TTL_S;
    s->paces_since_fight = 0;
    return false;
}

void digivice_expedition_walk(DigiviceState* s, uint32_t step, uint32_t t_end) {
    if(!digivice_expedition_can_walk(s)) return;
    uint32_t acc = s->pace_acc_s + step;
    s->pace_acc_s = acc % DIGIVICE_PACE_S;
    // Pace k happens when the carried awake seconds reach k minutes.
    for(uint32_t k = 1; k <= acc / DIGIVICE_PACE_S && digivice_expedition_can_walk(s); k++) {
        if(pace(s, t_end - (acc - k * DIGIVICE_PACE_S))) break;
    }
}

int32_t digivice_expedition_hike_phase(uint32_t ms, uint8_t* beat) {
    uint32_t n = (ms + DIGIVICE_HIKE_BEAT_MS / 2) / DIGIVICE_HIKE_BEAT_MS;
    if(n < 1) n = 1;
    if(n > DIGIVICE_HIKE_BEATS) n = DIGIVICE_HIKE_BEATS;
    if(beat) *beat = (uint8_t)(n - 1);
    return (int32_t)ms - (int32_t)(n * DIGIVICE_HIKE_BEAT_MS);
}

bool digivice_expedition_hike_tap_good(int32_t phase_ms) {
    return phase_ms >= -DIGIVICE_HIKE_GOOD_MS && phase_ms <= DIGIVICE_HIKE_GOOD_MS;
}

static bool window_expired(const DigiviceState* s, uint32_t now) {
    return s->bonus_window_start == 0 || now < s->bonus_window_start ||
           now - s->bonus_window_start >= DIGIVICE_BONUS_WINDOW_S;
}

static void bonus_window(DigiviceState* s, uint32_t now) {
    if(!window_expired(s, now)) return;
    s->bonus_window_start = now;
    s->hike_count = 0;
    s->scan_count = 0;
}

uint8_t digivice_expedition_hike_left(const DigiviceState* s, uint32_t now) {
    return window_expired(s, now) ? DIGIVICE_HIKE_CAP : DIGIVICE_HIKE_CAP - s->hike_count;
}

static uint8_t bonus_paces(DigiviceState* s, uint8_t want, uint32_t now) {
    uint8_t granted = 0;
    while(granted < want && digivice_expedition_can_walk(s)) {
        pace(s, now);
        granted++;
    }
    return granted;
}

uint8_t digivice_expedition_hike(DigiviceState* s, uint8_t good_taps, uint32_t now) {
    if(digivice_rules_is_asleep(now) || !digivice_expedition_can_walk(s)) return 0;
    bonus_window(s, now);
    uint8_t room = DIGIVICE_HIKE_CAP - s->hike_count;
    uint8_t granted = bonus_paces(s, good_taps < room ? good_taps : room, now);
    s->hike_count += granted;
    return granted;
}

uint8_t digivice_expedition_scan(DigiviceState* s, uint32_t now) {
    if(!digivice_expedition_can_walk(s)) return 0;
    bonus_window(s, now);
    uint8_t room = DIGIVICE_SCAN_CAP - s->scan_count;
    uint8_t granted =
        bonus_paces(s, DIGIVICE_SCAN_PACES < room ? DIGIVICE_SCAN_PACES : room, now);
    s->scan_count += granted;
    return granted;
}

bool digivice_expedition_all_clear(const DigiviceState* s) {
    return s->cleared == (1U << DIGIVICE_AREA_COUNT) - 1 && s->distance == 0;
}

bool digivice_expedition_boss_ready(const DigiviceState* s) {
    return s->distance == 0 && !digivice_expedition_all_clear(s);
}

bool digivice_expedition_boss_gate_open(const DigiviceState* s) {
    return digivice_rules_stage(s) >= DigiviceStageChampion;
}

static void start_area(DigiviceState* s, uint8_t area) {
    s->area = area;
    s->distance = digivice_rules_area_length(area);
    s->mid_state = 0;
    s->paces_since_fight = 0;
    s->enc_foe = DIGIVICE_ENC_NONE;
    s->enc_expires = 0;
}

void digivice_expedition_boss_won(DigiviceState* s) {
    s->cleared |= (uint8_t)(1U << clamp_area(s->area));
    for(uint8_t a = 0; a < DIGIVICE_AREA_COUNT; a++) {
        if(!(s->cleared & (1U << a))) {
            start_area(s, a);
            return;
        }
    }
    s->area = DIGIVICE_AREA_COUNT - 1;
    s->distance = 0;
}

void digivice_expedition_streak_lost(DigiviceState* s) {
    if(!digivice_expedition_streak_area(s->area)) return;
    s->distance = digivice_rules_area_length(s->area);
    s->mid_state = 0;
    s->paces_since_fight = 0;
}

bool digivice_expedition_select_area(DigiviceState* s, uint8_t area) {
    if(!digivice_expedition_area_unlocked(s, area)) return false;
    if(area != s->area) start_area(s, area);
    return true;
}

DigiviceMidwayResult digivice_expedition_midway_claim(
    DigiviceState* s,
    bool accept,
    uint8_t rng_pct,
    uint32_t now) {
    DigiviceMidwayResult result = {.kind = DigiviceMidwayCount, .ok = false};
    if(s->mid_state != 1) return result;
    s->mid_state = 2;
    result.kind = digivice_expedition_midway_kind(s->area);
    uint16_t length = digivice_rules_area_length(s->area);
    uint16_t jump = length / 5;
    switch(result.kind) {
    case DigiviceMidwaySpring:
        s->hunger = DIGIVICE_HUNGER_MAX;
        s->starving_since = 0;
        s->mood = DIGIVICE_MOOD_MAX;
        digivice_rules_add_corruption(s, -DIGIVICE_SPRING_CORRUPTION_DROP, now);
        s->injured_until = 0;
        result.ok = true;
        break;
    case DigiviceMidwayCache:
        if(s->item == DigiviceItemNone) {
            s->item = DigiviceItemShield;
            result.ok = true;
        } else {
            s->exp += DIGIVICE_CACHE_EXP;
        }
        break;
    case DigiviceMidwayShortcut:
        if(!accept) break;
        if(rng_pct < DIGIVICE_SHORTCUT_WIN_PCT) {
            s->distance = s->distance > jump ? s->distance - jump : 0;
            if(s->distance == 0) s->pace_acc_s = 0;
            result.ok = true;
        } else {
            s->distance = s->distance + jump < length ? s->distance + jump : length;
        }
        break;
    default:
        break;
    }
    return result;
}

bool digivice_rules_attention(const DigiviceState* s, uint32_t now) {
    return digivice_expedition_trail_pending(s, now) || digivice_rules_virux_active(s, now) ||
           s->mid_state == 1 ||
           (digivice_expedition_boss_ready(s) && digivice_expedition_boss_gate_open(s));
}
