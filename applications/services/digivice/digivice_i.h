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
    DigiviceMessageTypeClean,
    DigiviceMessageTypeCure,
    DigiviceMessageTypeToggleLight,
    DigiviceMessageTypeSetLang,
    DigiviceMessageTypeSetMode,
    DigiviceMessageTypeReset,
    DigiviceMessageTypeHike,
    DigiviceMessageTypeSelectArea,
    DigiviceMessageTypeMidway,
    DigiviceMessageTypeSetCity,
    DigiviceMessageTypeSetCompanion,
} DigiviceMessageType;

typedef struct {
    DigiviceMessageType type;
    FuriEventFlag* flag;
    bool* result;
    DigiviceResult* outcome;
    /** Typed result: DigiviceReward (Battle), uint8_t paces (Hike), DigiviceMidwayResult. */
    void* output;
    union {
        DolphinDeed deed;
        bool success;
        struct {
            uint8_t kind; /**< DigiviceEncounterKind */
            uint8_t result; /**< DigiviceBattleResult */
            bool shield_used;
            uint32_t offered_expires;
        } battle;
        uint8_t value;
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
