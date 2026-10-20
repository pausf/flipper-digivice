#pragma once

// Pure evolution tree: no furi includes, so the host test can compile this file with plain cc.

#include "digivice_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The form the pet evolves into at `now`, or DigiviceFormNone when it stays: the time in
 * stage has not elapsed, the form is an end form, or no branch condition holds yet. The egg
 * is not covered (its line comes from the rng, see digivice_rules_evolve). */
uint8_t digivice_evolution_next(const DigiviceState* s, uint32_t now);

#ifdef __cplusplus
}
#endif
