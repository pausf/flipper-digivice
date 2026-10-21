#pragma once
#include "furi.h"

// Both are driven by render.c: the clock is fixed per scene and the dice are seeded.
uint32_t furi_hal_random_get(void);
uint32_t furi_hal_rtc_get_timestamp(void);
