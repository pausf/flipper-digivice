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

/** One turn's dice. `acc`, `var`, `fx`, `para` and `miss` are 0..99, `crit` is 0..255. */
typedef struct {
    uint8_t acc; /**< the move hits when acc < move accuracy */
    uint8_t var; /**< damage roll, 0..99 maps to 85..100 percent */
    uint8_t crit; /**< a critical hit when crit % DIGIVICE_CRIT_ODDS == 0 */
    uint8_t fx; /**< the side effect lands when fx < its chance */
    uint8_t para; /**< a glitched fighter loses the turn when para < 25 */
    uint8_t miss; /**< corruption >= 50: the player's move fails when miss < 25 */
} DigiviceRolls;

typedef enum {
    DigiviceStepDone,
    DigiviceStepMissed,
    DigiviceStepSkipped, /**< lost to the glitch; no PP spent */
    DigiviceStepShielded, /**< the held Shield cancelled the foe's hit */
} DigiviceStepResult;

typedef struct {
    uint8_t actor; /**< DigiviceSide */
    uint8_t slot; /**< 0..3, DIGIVICE_SLOT_STRUGGLE after a fallback */
    uint8_t move; /**< DigiviceMoveId used */
    uint8_t result; /**< DigiviceStepResult */
    int8_t edge; /**< +1 super effective, -1 not very effective, 0 neutral */
    bool crit;
    uint8_t fx; /**< DigiviceFx that took effect, DigiviceFxNone when none or it failed */
    uint16_t damage;
    uint16_t healed;
} DigiviceStep;

typedef struct {
    uint8_t kind; /**< DigiviceEncounterKind */
    /** DigiviceFoe (pool), partner id, city monster index, area (boss) or DigiviceType (VIRUX) */
    uint8_t foe;
    uint8_t foe_class; /**< DigiviceFoeClass */
    uint8_t round; /**< completed rounds */
    bool item_shield; /**< the pet holds a Shield */
    bool shield_armed; /**< OBJETO spent a turn: the next foe hit is cancelled */
    bool shield_used; /**< the Shield cancelled a hit: the item is consumed */
    bool glitchy; /**< corruption >= 50 */
    DigiviceFighter f[DigiviceSideCount];
} DigiviceDuel;

void digivice_duel_init(
    DigiviceDuel* d,
    const DigiviceState* s,
    DigiviceEncounterKind kind,
    uint32_t now);
/** LUCHAR/OBJETO: true when a Shield is held and not armed yet. */
bool digivice_duel_shield_ready(const DigiviceDuel* d);
/** Arms the Shield (the caller spends the turn). False when it is not ready. */
bool digivice_duel_arm_shield(DigiviceDuel* d);
/** Who acts first: move priority, then speed (halved while glitched), the pet on a tie. */
bool digivice_duel_player_first(const DigiviceDuel* d, uint8_t player_slot, uint8_t foe_slot);
/** Resolves one move. `crit_bonus` is the timing bar's guaranteed critical hit. */
DigiviceStep digivice_duel_act(
    DigiviceDuel* d,
    uint8_t actor,
    uint8_t slot,
    bool crit_bonus,
    const DigiviceRolls* rolls);
void digivice_duel_end_round(DigiviceDuel* d);
/** A KO ends it; after DIGIVICE_TURN_CAP rounds the higher HP fraction wins, a tie is a draw. */
DigiviceBattleOutcome digivice_duel_outcome(const DigiviceDuel* d);

#ifdef __cplusplus
}
#endif
