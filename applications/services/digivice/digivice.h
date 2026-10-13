#pragma once

#include <core/pubsub.h>

#include "digivice_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_DIGIVICE "digivice"

typedef struct Digivice Digivice;

/** Pubsub message type: every publish carries a `const DigiviceState*` snapshot. */
FuriPubSub* digivice_get_pubsub(Digivice* digivice);

/** Copies the current state. Thread safe, never waits on the service queue. */
void digivice_get_state(Digivice* digivice, DigiviceState* state);

/** Thread safe, blocking. Returns false when the pet was already full. */
bool digivice_feed(Digivice* digivice);

/** Thread safe, blocking. */
void digivice_train(Digivice* digivice, bool success);

/** Thread safe, blocking. Removes the pending VIRUX and applies rewards or penalties. */
void digivice_battle_finish(Digivice* digivice, DigiviceBattleResult result);

/** Thread safe, never blocks: if the queue is full the cutscene simply plays again later. */
void digivice_ack_evolution(Digivice* digivice);

#ifdef __cplusplus
}
#endif
