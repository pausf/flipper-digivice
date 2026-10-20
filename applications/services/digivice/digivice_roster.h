#pragma once

// Pure form table: no furi includes, so the host test can compile this file with plain cc.

#include "digivice_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* name; /**< proper noun, never translated, at most 9 glyphs */
    uint8_t stage; /**< DigiviceStage */
    uint8_t attr; /**< DigiviceAttr */
    uint32_t stay_s; /**< time in this form before it can evolve, 0 = never */
} DigiviceRosterEntry;

/** NULL when `form` is out of range. */
const DigiviceRosterEntry* digivice_roster(uint8_t form);

#ifdef __cplusplus
}
#endif
