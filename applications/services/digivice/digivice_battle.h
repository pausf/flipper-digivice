#pragma once

// Pure battle rules: no furi includes, so the host test can compile this file with plain cc.

#include "digivice_expedition.h"

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

/** Aim and guard heights share these values. */
typedef enum {
    DigiviceAimHigh,
    DigiviceAimLow,
} DigiviceAim;

typedef enum {
    DigiviceShotHit, /**< reached the target */
    DigiviceShotDodge, /**< missed: the target hopped away */
    DigiviceShotGuardBroken, /**< blocked height, but the halved roll still hit */
    DigiviceShotBlocked, /**< died on the guard */
    DigiviceShotShielded, /**< foe hit cancelled by the Shield */
} DigiviceShot;

typedef struct {
    uint16_t exp;
    uint8_t item; /**< DigiviceItem dropped, DigiviceItemNone when nothing dropped */
} DigiviceReward;

typedef struct {
    uint8_t kind; /**< DigiviceEncounterKind */
    uint8_t foe; /**< DigiviceFoe (trail), area (boss) or DigiviceType (VIRUX) */
    uint8_t player_attr; /**< DigiviceAttr */
    uint8_t foe_attr;
    uint8_t player_shot; /**< projectile size in px, DIGIVICE_SHOT_BEAM for the beam */
    uint8_t foe_shot;
    uint8_t player_hits;
    uint8_t foe_hits;
    uint8_t round; /**< completed rounds: one player shot and one foe shot each */
    bool shield; /**< the held Shield still waits for the first foe hit */
    bool shield_used;
    bool glitchy; /**< corruption >= 50: every player shot first rolls the corruption miss */
    uint16_t player_power; /**< uncharged power P */
    uint16_t foe_power; /**< Q */
} DigiviceBattle;

/** round(100*P/(P+Q)) + 5*adv, clamped to 10..90. */
uint8_t digivice_rules_hit_pct(uint16_t p, uint16_t q, int8_t adv);
/** Stage base power plus strength (capped at 24), x3/4 while injured. */
uint16_t digivice_rules_player_power(const DigiviceState* s, uint32_t now);
uint16_t digivice_rules_charged_power(uint16_t power, DigiviceCharge tier);
/** Zone width on the 20-segment charge meter: 3 + min(strength, 24) / 8. */
uint8_t digivice_rules_charge_zone_w(uint16_t strength);
/** Ping-pong cursor of the charge meter: 0..19..0, one segment per frame. */
uint8_t digivice_rules_meter_cursor(uint32_t frame);
/** Random zone start in [3, 20 - w - 1]. */
uint8_t digivice_rules_charge_zone_start(uint8_t zone_w, uint8_t rng);
/** Big inside the zone, Normal within 2 segments of it, Small otherwise. */
DigiviceCharge digivice_rules_charge_tier(uint8_t cursor, uint8_t zone_start, uint8_t zone_w);
/** Guard height at `frame` of the aim phase: flips every 5 frames from g0. */
uint8_t digivice_rules_guard_at(uint8_t g0, uint32_t frame);
/** The guard is sampled when the shot lands, 5 frames after the press. */
bool digivice_rules_shot_blocked(uint8_t aim, uint8_t g0, uint32_t press_frame);
bool digivice_rules_injured(const DigiviceState* s, uint32_t now);

void digivice_battle_init(
    DigiviceBattle* b,
    const DigiviceState* s,
    DigiviceEncounterKind kind,
    uint32_t now);
/** `miss_roll` feeds the corruption miss, `hit_roll` the hit chance; both 0..99. */
DigiviceShot digivice_battle_player_shot(
    DigiviceBattle* b,
    DigiviceCharge tier,
    bool blocked,
    uint8_t miss_roll,
    uint8_t hit_roll);
/** The automatic foe turn at the player's uncharged power; ends the round. */
DigiviceShot digivice_battle_foe_shot(DigiviceBattle* b, uint8_t hit_roll);
/** First to 3 hits; after round 10 the side with more hits wins, a tie is a draw. */
DigiviceBattleOutcome digivice_battle_outcome(const DigiviceBattle* b);
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
