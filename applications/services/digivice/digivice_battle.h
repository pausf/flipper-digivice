#pragma once

// Pure battle rules: no furi includes, so the host test can compile this file with plain cc.

#include "digivice_expedition.h"
#include "digivice_homage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigiviceBattleResultWin,
    DigiviceBattleResultLose,
    DigiviceBattleResultFled,
    DigiviceBattleResultDraw,
} DigiviceBattleResult;

typedef enum {
    DigiviceBattleOngoing,
    DigiviceBattleWon,
    DigiviceBattleLost,
    DigiviceBattleDrawn,
} DigiviceBattleOutcome;

typedef enum {
    DigiviceEncounterTrail,
    DigiviceEncounterBoss,
    DigiviceEncounterVirux,
    DigiviceEncounterCount,
} DigiviceEncounterKind;

typedef enum {
    DigiviceChargeSmall,
    DigiviceChargeNormal,
    DigiviceChargeBig,
    DigiviceChargeCount,
} DigiviceCharge;

typedef enum {
    DigiviceFoeClassPool, /**< `foe` is a DigiviceFoe */
    DigiviceFoeClassHomage, /**< `foe` is the partner id */
    DigiviceFoeClassCity, /**< `foe` is the city monster index */
} DigiviceFoeClass;

typedef struct {
    uint16_t exp;
    uint8_t item; /**< DigiviceItem dropped, DigiviceItemNone when nothing dropped */
    uint8_t capture; /**< DigiviceCapture */
    uint8_t partner; /**< partner id when capture != None */
} DigiviceReward;

/** Stage base power plus strength (capped at 24), x3/4 while injured. */
uint16_t digivice_rules_player_power(const DigiviceState* s, uint32_t now);
/** Zone width on the 20-segment charge meter: 3 + min(strength, 24) / 8. */
uint8_t digivice_rules_charge_zone_w(uint16_t strength);
/** Ping-pong cursor of the charge meter: 0..19..0, one segment per frame. */
uint8_t digivice_rules_meter_cursor(uint32_t frame);
/** Random zone start in [3, 20 - w - 1]. */
uint8_t digivice_rules_charge_zone_start(uint8_t zone_w, uint8_t rng);
/** Big inside the zone, Normal within 2 segments of it, Small otherwise. */
DigiviceCharge digivice_rules_charge_tier(uint8_t cursor, uint8_t zone_start, uint8_t zone_w);
bool digivice_rules_injured(const DigiviceState* s, uint32_t now);
/** VIRUX and areas 1 to 4 allow a retreat; streak areas do not. */
bool digivice_battle_retreat_allowed(const DigiviceState* s, DigiviceEncounterKind kind);

/** Applies a finished battle and clears its encounter. `shield_used` drops the held Shield.
 * A trail encounter is only cleared when it still expires at `offered_expires`, the value the
 * fight started with. */
DigiviceReward digivice_rules_battle_result(
    DigiviceState* s,
    DigiviceEncounterKind kind,
    DigiviceBattleResult result,
    bool shield_used,
    uint32_t offered_expires,
    uint8_t rng_pct,
    uint32_t now);

#ifdef __cplusplus
}
#endif
