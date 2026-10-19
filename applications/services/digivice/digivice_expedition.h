#pragma once

// Pure Expedition rules: no furi includes, so the host test can compile this file with plain cc.

#include "digivice_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The eight trail monsters. Proper nouns: never translated. */
typedef enum {
    DigiviceFoeEstatik,
    DigiviceFoeChipko,
    DigiviceFoeKorno,
    DigiviceFoeMokobit,
    DigiviceFoeFuzzcan,
    DigiviceFoeVampixel,
    DigiviceFoeSilbapo,
    DigiviceFoeChatarro,
    DigiviceFoeCount,
} DigiviceFoe;

typedef enum {
    DigiviceMidwaySpring,
    DigiviceMidwayCache,
    DigiviceMidwayShortcut,
    DigiviceMidwayCount, /**< also "nothing was claimed" in DigiviceMidwayResult */
} DigiviceMidwayKind;

typedef struct {
    uint8_t kind; /**< DigiviceMidwayKind, DigiviceMidwayCount when nothing was available */
    bool ok; /**< Spring: always; Cache: a Shield (false: EXP); Shortcut: the gamble paid */
} DigiviceMidwayResult;

DigiviceFoe digivice_expedition_pool_foe(uint8_t area, uint8_t index);
DigiviceAttr digivice_expedition_foe_attr(DigiviceFoe foe);
/** FUZZCAN, VAMPIXEL, SILBAPO and CHATARRO are Champion-tier; the rest Rookie-tier. */
bool digivice_expedition_foe_champion(DigiviceFoe foe);
const char* digivice_expedition_foe_name(DigiviceFoe foe);
/** Trail foe power: area power x1.0 (Rookie-tier) or x1.3 (Champion-tier), floored. */
uint16_t digivice_expedition_foe_power(uint8_t area, DigiviceFoe foe);
uint16_t digivice_expedition_boss_power(uint8_t area);
DigiviceAttr digivice_expedition_boss_attr(uint8_t area);
const char* digivice_expedition_boss_name(uint8_t area);
DigiviceMidwayKind digivice_expedition_midway_kind(uint8_t area);
/** Areas 5 to 8: no retreat, and any loss restarts the area. */
bool digivice_expedition_streak_area(uint8_t area);
/** Area 1 is always open; area i needs boss i-1. */
bool digivice_expedition_area_unlocked(const DigiviceState* s, uint8_t area);

/** Paces count for an awake pet that is at least Baby, not sick and short of the boss. */
bool digivice_expedition_can_walk(const DigiviceState* s);
/** Called by the tick for every awake step that ends at t_end. */
void digivice_expedition_walk(DigiviceState* s, uint32_t step, uint32_t t_end);
/** Clears a trail encounter whose time ran out. No penalty. */
void digivice_expedition_expire(DigiviceState* s, uint32_t now);
bool digivice_expedition_trail_pending(const DigiviceState* s, uint32_t now);

/** Signed distance in ms from `ms` (since the session start) to the nearest beat. */
int32_t digivice_expedition_hike_phase(uint32_t ms, uint8_t* beat);
bool digivice_expedition_hike_tap_good(int32_t phase_ms);
/** Hike paces still available in the current hourly window. */
uint8_t digivice_expedition_hike_left(const DigiviceState* s, uint32_t now);
/** Grants up to `good_taps` paces within the hourly cap. Returns the paces granted. */
uint8_t digivice_expedition_hike(DigiviceState* s, uint8_t good_taps, uint32_t now);
/** Hacker scan deed: DIGIVICE_SCAN_PACES paces within the hourly cap. Returns the paces. */
uint8_t digivice_expedition_scan(DigiviceState* s, uint32_t now);

/** The boss waits at distance 0, except once every area is cleared. */
bool digivice_expedition_boss_ready(const DigiviceState* s);
bool digivice_expedition_all_clear(const DigiviceState* s);
/** Champion or above (Virubyte counts as its prev_form). */
bool digivice_expedition_boss_gate_open(const DigiviceState* s);
/** Marks the area cleared and moves to the lowest uncleared area. */
void digivice_expedition_boss_won(DigiviceState* s);
/** A lost fight in a streak area restarts it; areas 1 to 4 keep their progress. */
void digivice_expedition_streak_lost(DigiviceState* s);
/** False for a locked area. Switching areas restarts the new one from its start. */
bool digivice_expedition_select_area(DigiviceState* s, uint8_t area);
/** Claims the available midway event; `accept` only matters for the shortcut. */
DigiviceMidwayResult digivice_expedition_midway_claim(
    DigiviceState* s,
    bool accept,
    uint8_t rng_pct,
    uint32_t now);

/** Something waits for the player: an encounter, a VIRUX, a midway event or an open boss. */
bool digivice_rules_attention(const DigiviceState* s, uint32_t now);

#ifdef __cplusplus
}
#endif
