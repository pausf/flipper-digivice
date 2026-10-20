#pragma once

#include <core/pubsub.h>

#include "digivice_battle.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_DIGIVICE "digivice"

typedef struct Digivice Digivice;

/** Pubsub message type: every publish carries a `const DigiviceState*` snapshot. */
FuriPubSub* digivice_get_pubsub(Digivice* digivice);

/** Copies the current state. Thread safe, never waits on the service queue. */
void digivice_get_state(Digivice* digivice, DigiviceState* state);

/** Thread safe, blocking. Returns false when the food was refused (the pet is full of it). */
bool digivice_feed(Digivice* digivice, DigiviceFood food);

/** Thread safe, blocking. Refused (Sick, Asleep) without changing anything. */
DigiviceResult digivice_train(Digivice* digivice, bool success);

/** Thread safe, blocking. Applies a finished battle, clears its trail encounter or VIRUX and
 * returns the reward. `shield_used` drops the Shield the fight consumed; `offered_expires`
 * is the trail encounter's expiry when the fight started. */
DigiviceReward digivice_battle_finish(
    Digivice* digivice,
    DigiviceEncounterKind kind,
    DigiviceBattleResult result,
    bool shield_used,
    uint32_t offered_expires);

/** Thread safe, blocking. NothingToClean when there was no poop. */
DigiviceResult digivice_clean(Digivice* digivice);

/** Thread safe, blocking. Uses one dose; Healthy when the pet was not sick. */
DigiviceResult digivice_cure(Digivice* digivice);

/** Thread safe, blocking. Awake when the pet is awake: the light only goes off at night. */
DigiviceResult digivice_toggle_light(Digivice* digivice);

/** Thread safe, blocking. Out-of-range values are ignored. */
void digivice_set_lang(Digivice* digivice, DigiviceLang lang);
void digivice_set_mode(Digivice* digivice, DigiviceMode mode);

/** Thread safe, blocking. Replaces the pet with a fresh egg; language and mode stay. */
void digivice_reset(Digivice* digivice);

/** Thread safe, blocking. Grants Hike paces within the hourly cap; returns the paces granted.
 * Hike paces are bookkeeping, so the service saves them explicitly. */
uint8_t digivice_hike_finish(Digivice* digivice, uint8_t good_taps);

/** Thread safe, blocking. False (and nothing changes) when the area is locked. */
bool digivice_select_area(Digivice* digivice, uint8_t area);

/** Thread safe, blocking. Claims the available midway event; `accept` answers the shortcut. */
DigiviceMidwayResult digivice_midway_claim(Digivice* digivice, bool accept);

/** Thread safe, never blocks: if the queue is full the cutscene simply plays again later. */
void digivice_ack_evolution(Digivice* digivice);

#ifdef __cplusplus
}
#endif
