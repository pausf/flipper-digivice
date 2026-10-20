#pragma once

// Homage partners (private build, flag DIGIVICE_EASTER_EGGS). Pure C: no furi includes. Partners
// are ids 0..DIGIVICE_HOMAGE_COUNT-1; names live only in digivice_homage.c.

#include "digivice_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigiviceCaptureNone,
    DigiviceCaptureCaught,
    DigiviceCaptureEscaped,
    DigiviceCaptureFriend, /**< already in the nest */
} DigiviceCapture;

bool digivice_homage_enabled(void);
/** True when `code` is an enc_foe value of a partner (always false with the flag off). */
bool digivice_homage_code(uint8_t code);
/** Proper noun, "?" when out of range or disabled. */
const char* digivice_homage_name(uint8_t id);
DigiviceAttr digivice_homage_attr(uint8_t id);
/** Chance in percent that a spawning trail encounter is a partner, by special_count. */
uint8_t digivice_homage_odds(uint8_t count);
/** Rolls the partner for a spawning trail encounter. Returns its enc_foe code or
 * DIGIVICE_ENC_NONE. On success special_count resets to 0. Never advances s->rng. */
uint8_t digivice_homage_pick(DigiviceState* s);
/** Counts a finished Win or Lose that was not a partner fight (saturating). */
void digivice_homage_count_battle(DigiviceState* s);
/** Chance in percent to catch a beaten partner. */
uint8_t digivice_homage_capture_pct(const DigiviceState* s);
/** Resolves a won partner fight with a 0..99 roll: updates caught, companion, special_count. */
DigiviceCapture digivice_homage_resolve(DigiviceState* s, uint8_t id, uint8_t roll);

#ifdef __cplusplus
}
#endif
