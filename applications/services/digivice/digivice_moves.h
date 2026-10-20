#pragma once

// Pure move table (v3.4): no furi includes, so the host test can compile this file with plain cc.

#include "digivice_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigiviceFxNone,
    DigiviceFxAtkDown, /**< target ATK down one step, `arg` is the chance in percent */
    DigiviceFxDefDown, /**< target DEF down one step */
    DigiviceFxHeal, /**< user heals `arg` percent of its max HP */
    DigiviceFxCharge, /**< the user's next damaging move deals x2 */
    DigiviceFxGlitch, /**< target glitched: loses 25% of its turns and half its speed */
    DigiviceFxShield, /**< halves the next damage the user takes */
    DigiviceFxQuick, /**< priority +1 */
    DigiviceFxCount,
} DigiviceFx;

typedef enum {
    DigiviceMoveZarpazo,
    DigiviceMoveMordisco,
    DigiviceMoveEmbestida,
    DigiviceMoveReparar,
    DigiviceMoveEscudoDatos,
    DigiviceMoveSobrecarga,
    DigiviceMoveRafaga,
    DigiviceMoveGrito,
    DigiviceMoveFragmentar,
    DigiviceMoveLucha, /**< the fallback when every PP is spent; never learned */
    /** 16 attribute moves: DigiviceMoveKit + attr * 4 + {mid, heavy, effect A, effect B}. */
    DigiviceMoveKit,
    DigiviceMoveCount = DigiviceMoveKit + 4 * DigiviceAttrCount,
} DigiviceMoveId;

#define DIGIVICE_MOVE_NONE 0xFF

typedef struct {
    const char* name; /**< ASCII, upper case, at most 12 glyphs, never translated */
    uint8_t power; /**< 0 for a status move */
    uint8_t acc; /**< percent; 0 = no accuracy check */
    uint8_t pp;
    uint8_t attr; /**< DigiviceAttr */
    uint8_t fx; /**< DigiviceFx */
    uint8_t arg; /**< side-effect chance (damaging move) or heal percent */
} DigiviceMove;

/** Never NULL: an out-of-range id answers DigiviceMoveLucha. */
const DigiviceMove* digivice_move(uint8_t id);

/** Fills the four slots of a fighter. `tier` 0 Baby/Baby II, 1 Rookie, 2 Champion, 3 Ultimate,
 * 4 Mega. `seed` varies the effect move and the utility move between forms of one stage and
 * attribute. `force_util` replaces slot 3 (bosses, VIRUX), DIGIVICE_MOVE_NONE keeps the rule. */
void digivice_moves_assign(
    uint8_t tier,
    uint8_t attr,
    uint8_t seed,
    uint8_t force_util,
    uint8_t out[4]);

#ifdef __cplusplus
}
#endif
