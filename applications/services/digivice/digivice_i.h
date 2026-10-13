#pragma once

#include <furi.h>
#include <dolphin/dolphin.h>
#include <notification/notification.h>
#include <storage/storage.h>

#include "digivice.h"

typedef enum {
    DigiviceMessageTypeDeed,
    DigiviceMessageTypeFeed,
    DigiviceMessageTypeTrain,
    DigiviceMessageTypeBattle,
    DigiviceMessageTypeAckEvolution,
    DigiviceMessageTypeReload,
} DigiviceMessageType;

typedef struct {
    DigiviceMessageType type;
    FuriEventFlag* flag;
    bool* result;
    union {
        DolphinDeed deed;
        bool success;
        DigiviceBattleResult battle;
    };
} DigiviceMessage;

struct Digivice {
    DigiviceState state;
    FuriMutex* mutex;
    FuriPubSub* pubsub;
    FuriMessageQueue* queue;
    FuriEventLoop* event_loop;
    FuriEventLoopTimer* tick_timer;
    FuriEventLoopTimer* flush_timer;
    FuriPubSubSubscription* deed_subscription;
    FuriPubSubSubscription* storage_subscription;
    NotificationApp* notification;
    Storage* storage;
    bool storage_ready;
    bool dirty;
};
