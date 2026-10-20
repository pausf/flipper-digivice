#pragma once

// Pure duel rules (v3.4): stats, damage, foe AI and the turn state machine. No furi includes.

#include "digivice_battle.h"
#include "digivice_moves.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigiviceSidePlayer,
    DigiviceSideFoe,
    DigiviceSideCount,
} DigiviceSide;

#define DIGIVICE_SLOTS          4
#define DIGIVICE_SLOT_STRUGGLE  4

typedef struct {
    uint16_t hp;
    uint16_t hp_max;
    uint16_t atk;
    uint16_t def;
    uint16_t spd;
    uint8_t level;
    uint8_t attr; /**< DigiviceAttr */
    uint8_t shot; /**< projectile size in px */
    uint8_t moves[DIGIVICE_SLOTS];
    uint8_t pp[DIGIVICE_SLOTS];
    uint8_t atk_low; /**< steps the ATK is lowered, 0..DIGIVICE_STAGE_MAX */
    uint8_t def_low;
    bool glitch;
    bool guard; /**< ESCUDO DATOS: halves the next damage taken */
    bool charged; /**< SOBRECARGA: the next damaging move deals x2 */
} DigiviceFighter;

typedef struct {
    uint16_t power; /**< the v3.1 power P, the only stat input */
    uint8_t attr;
    uint8_t tier; /**< see digivice_moves_assign */
    uint8_t seed;
    uint8_t force_util; /**< DIGIVICE_MOVE_NONE for the plain rule */
    uint8_t spd_bonus;
    uint8_t shot;
} DigiviceFighterSpec;

/** Baby, Baby II 0; Rookie 1; Champion 2; Ultimate 3; Mega 4. */
uint8_t digivice_duel_tier(DigiviceStage stage);
void digivice_fighter_init(DigiviceFighter* f, const DigiviceFighterSpec* spec);
/** Damage of one hit with `var` 0..99 mapping to a roll of 85..100 percent. Always at least 1.
 * Charge and guard are applied by digivice_duel_act. */
uint16_t digivice_duel_damage(
    const DigiviceFighter* attacker,
    const DigiviceFighter* defender,
    const DigiviceMove* move,
    bool crit,
    uint8_t var);
/** Slot 0..3, or DIGIVICE_SLOT_STRUGGLE when nothing is worth or possible. `roll` 0..255. */
uint8_t digivice_fighter_pick(const DigiviceFighter* me, const DigiviceFighter* foe, uint8_t roll);
/** True when no slot has PP left: the only move is LUCHA. */
bool digivice_fighter_spent(const DigiviceFighter* f);

#ifdef __cplusplus
}
#endif
