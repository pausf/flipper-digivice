#include "digivice_i.h"
#include "digivice_config.h"
#include "digivice_notify.h"

#include <furi_hal.h>
#include <toolbox/saved_struct.h>

#define TAG "Digivice"

#define DIGIVICE_STATE_PATH    INT_PATH(".digivice.state")
#define DIGIVICE_STATE_MAGIC      0xD1
#define DIGIVICE_STATE_VERSION    0x02
#define DIGIVICE_STATE_VERSION_V1 0x01

#define DIGIVICE_QUEUE_SIZE 16
#define DIGIVICE_DONE_FLAG  (0x1)

// Public API

FuriPubSub* digivice_get_pubsub(Digivice* digivice) {
    furi_check(digivice);
    return digivice->pubsub;
}

void digivice_get_state(Digivice* digivice, DigiviceState* state) {
    furi_check(digivice);
    furi_check(state);
    furi_check(furi_mutex_acquire(digivice->mutex, FuriWaitForever) == FuriStatusOk);
    *state = digivice->state;
    furi_mutex_release(digivice->mutex);
}

static void digivice_send_wait(Digivice* digivice, DigiviceMessage* message) {
    message->flag = furi_event_flag_alloc();
    furi_check(furi_message_queue_put(digivice->queue, message, FuriWaitForever) == FuriStatusOk);
    furi_check(
        furi_event_flag_wait(message->flag, DIGIVICE_DONE_FLAG, FuriFlagWaitAny, FuriWaitForever) ==
        DIGIVICE_DONE_FLAG);
    furi_event_flag_free(message->flag);
}

bool digivice_feed(Digivice* digivice) {
    furi_check(digivice);
    bool fed = false;
    DigiviceMessage message = {.type = DigiviceMessageTypeFeed, .result = &fed};
    digivice_send_wait(digivice, &message);
    return fed;
}

static DigiviceResult digivice_send_outcome(Digivice* digivice, DigiviceMessage* message) {
    DigiviceResult outcome = DigiviceResultOk;
    message->outcome = &outcome;
    digivice_send_wait(digivice, message);
    return outcome;
}

DigiviceResult digivice_train(Digivice* digivice, bool success) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeTrain, .success = success};
    return digivice_send_outcome(digivice, &message);
}

DigiviceResult digivice_clean(Digivice* digivice) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeClean};
    return digivice_send_outcome(digivice, &message);
}

DigiviceResult digivice_cure(Digivice* digivice) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeCure};
    return digivice_send_outcome(digivice, &message);
}

DigiviceResult digivice_toggle_light(Digivice* digivice) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeToggleLight};
    return digivice_send_outcome(digivice, &message);
}

void digivice_set_lang(Digivice* digivice, DigiviceLang lang) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeSetLang, .value = (uint8_t)lang};
    digivice_send_wait(digivice, &message);
}

void digivice_set_mode(Digivice* digivice, DigiviceMode mode) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeSetMode, .value = (uint8_t)mode};
    digivice_send_wait(digivice, &message);
}

void digivice_reset(Digivice* digivice) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeReset};
    digivice_send_wait(digivice, &message);
}

void digivice_battle_finish(Digivice* digivice, DigiviceBattleResult result) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeBattle, .battle = result};
    digivice_send_wait(digivice, &message);
}

void digivice_ack_evolution(Digivice* digivice) {
    furi_check(digivice);
    DigiviceMessage message = {.type = DigiviceMessageTypeAckEvolution};
    furi_message_queue_put(digivice->queue, &message, 0);
}

// Private

/** Reads a v2 file, or a v1 file migrated to v2 (`migrated` is then set). */
static bool digivice_load_file(DigiviceState* loaded, bool* migrated) {
    uint8_t magic = 0;
    uint8_t version = 0;
    size_t size = 0;
    *migrated = false;
    if(!saved_struct_get_metadata(DIGIVICE_STATE_PATH, &magic, &version, &size)) return false;
    if(magic != DIGIVICE_STATE_MAGIC) return false;

    if(version == DIGIVICE_STATE_VERSION && size == sizeof(DigiviceState)) {
        return saved_struct_load(
            DIGIVICE_STATE_PATH,
            loaded,
            sizeof(DigiviceState),
            DIGIVICE_STATE_MAGIC,
            DIGIVICE_STATE_VERSION);
    }
    if(version == DIGIVICE_STATE_VERSION_V1 && size == sizeof(DigiviceStateV1)) {
        DigiviceStateV1 v1;
        if(!saved_struct_load(
               DIGIVICE_STATE_PATH,
               &v1,
               sizeof(DigiviceStateV1),
               DIGIVICE_STATE_MAGIC,
               DIGIVICE_STATE_VERSION_V1)) {
            return false;
        }
        digivice_rules_migrate_v1(&v1, loaded);
        *migrated = true;
        return true;
    }
    return false;
}

static void digivice_load(Digivice* digivice) {
    DigiviceState loaded;
    bool migrated = false;
    bool ok = digivice_load_file(&loaded, &migrated);
    if(ok && (loaded.form >= DigiviceFormCount || loaded.prev_form >= DigiviceFormCount)) {
        ok = false;
    }
    if(ok) digivice_rules_sanitize(&loaded);

    furi_check(furi_mutex_acquire(digivice->mutex, FuriWaitForever) == FuriStatusOk);
    if(ok) {
        // v1 time was never played under the v2 care rules: restart the clock so the first tick
        // sees ~0 s elapsed instead of charging mistakes, poop and sickness for it.
        if(migrated) loaded.last_tick = furi_hal_rtc_get_timestamp();
        digivice->state = loaded;
        // A migrated pet is written back as v2 on the next flush.
        digivice->dirty = migrated;
        if(migrated) FURI_LOG_I(TAG, "Migrated the v1 state to v2");
    } else {
        FURI_LOG_W(TAG, "No valid state, hatching a new egg");
        digivice_rules_init(&digivice->state, furi_hal_rtc_get_timestamp());
        digivice->dirty = true;
    }
    furi_mutex_release(digivice->mutex);
    digivice->storage_ready = true;
    if(digivice->dirty)
        furi_event_loop_timer_start(digivice->flush_timer, DIGIVICE_FLUSH_S * 1000UL);
}

static void digivice_save(Digivice* digivice) {
    if(!digivice->dirty || !digivice->storage_ready) return;

    DigiviceState snapshot;
    digivice_get_state(digivice, &snapshot);
    if(saved_struct_save(
           DIGIVICE_STATE_PATH,
           &snapshot,
           sizeof(DigiviceState),
           DIGIVICE_STATE_MAGIC,
           DIGIVICE_STATE_VERSION)) {
        digivice->dirty = false;
    } else {
        FURI_LOG_E(TAG, "Failed to save state");
        furi_event_loop_timer_start(digivice->flush_timer, DIGIVICE_FLUSH_S * 1000UL);
    }
}

static void digivice_publish(Digivice* digivice) {
    DigiviceState snapshot;
    digivice_get_state(digivice, &snapshot);
    furi_pubsub_publish(digivice->pubsub, &snapshot);
}

/** Runs `before` vs current state bookkeeping after any mutation done under the mutex. */
static void digivice_changed(Digivice* digivice, const DigiviceState* before) {
    DigiviceState after;
    digivice_get_state(digivice, &after);
    if(digivice_rules_same_visible(before, &after)) return;

    bool asleep = digivice_rules_is_asleep(furi_hal_rtc_get_timestamp());
    if(after.evo_from != DigiviceFormNone && after.evo_from != before->evo_from) {
        notification_message(
            digivice->notification,
            asleep ? &digivice_sequence_evolution_silent : &digivice_sequence_evolution);
    } else if(after.sick && !before->sick) {
        notification_message(
            digivice->notification,
            asleep ? &digivice_sequence_sick_silent : &digivice_sequence_sick);
    } else if(after.poop > before->poop && !asleep) {
        notification_message(digivice->notification, &digivice_sequence_poop);
    }
    digivice->dirty = true;
    furi_event_loop_timer_start(digivice->flush_timer, DIGIVICE_FLUSH_S * 1000UL);
    digivice_publish(digivice);
}

static void digivice_tick(void* context) {
    Digivice* digivice = context;

    if(!digivice->storage_ready && storage_sd_status(digivice->storage) == FSE_OK) {
        digivice_load(digivice);
        digivice_publish(digivice);
    }

    uint32_t now = furi_hal_rtc_get_timestamp();
    furi_check(furi_mutex_acquire(digivice->mutex, FuriWaitForever) == FuriStatusOk);
    DigiviceState before = digivice->state;
    digivice_rules_tick(&digivice->state, now);
    digivice_rules_evolve(&digivice->state, now);
    furi_mutex_release(digivice->mutex);

    digivice_changed(digivice, &before);
}

static void digivice_flush(void* context) {
    digivice_save(context);
}

static void digivice_process_message(FuriEventLoopObject* object, void* context) {
    UNUSED(object);
    Digivice* digivice = context;
    DigiviceMessage message;
    furi_check(furi_message_queue_get(digivice->queue, &message, 0) == FuriStatusOk);

    if(message.type == DigiviceMessageTypeReload) {
        // Only the first mount loads; later remounts keep the live pet and save over the card.
        if(!digivice->storage_ready) {
            digivice_load(digivice);
            digivice_publish(digivice);
        }
        return;
    }

    uint32_t now = furi_hal_rtc_get_timestamp();
    bool spawned = false;

    furi_check(furi_mutex_acquire(digivice->mutex, FuriWaitForever) == FuriStatusOk);
    DigiviceState before = digivice->state;
    DigiviceState* state = &digivice->state;

    switch(message.type) {
    case DigiviceMessageTypeDeed:
        // Test deeds sit above DolphinDeedMAX and have no weight entry.
        if(message.deed < DolphinDeedMAX) {
            spawned = digivice_rules_on_deed(
                state,
                message.deed,
                dolphin_deed_get_weight(message.deed),
                now,
                (uint8_t)(furi_hal_random_get() % 100));
            digivice_rules_evolve(state, now);
            FURI_LOG_D(TAG, "Deed %d: exp %lu", (int)message.deed, (unsigned long)state->exp);
        }
        break;
    case DigiviceMessageTypeFeed:
        *message.result = digivice_rules_feed(state, now);
        break;
    case DigiviceMessageTypeTrain:
        *message.outcome = digivice_rules_train(state, message.success, now);
        digivice_rules_evolve(state, now);
        break;
    case DigiviceMessageTypeClean:
        *message.outcome = digivice_rules_clean(state, now) ? DigiviceResultOk :
                                                             DigiviceResultNothingToClean;
        break;
    case DigiviceMessageTypeCure:
        *message.outcome = digivice_rules_cure(state, now) ? DigiviceResultOk :
                                                            DigiviceResultHealthy;
        break;
    case DigiviceMessageTypeToggleLight:
        *message.outcome = digivice_rules_toggle_light(state, now);
        break;
    case DigiviceMessageTypeSetLang:
        digivice_rules_set_lang(state, message.value);
        break;
    case DigiviceMessageTypeSetMode:
        digivice_rules_set_mode(state, message.value);
        break;
    case DigiviceMessageTypeReset:
        digivice_rules_reset(state, now);
        break;
    case DigiviceMessageTypeBattle:
        digivice_rules_battle_result(state, message.battle, now);
        digivice_rules_evolve(state, now);
        break;
    case DigiviceMessageTypeAckEvolution:
        digivice_rules_ack_evolution(state);
        break;
    default:
        break;
    }
    furi_mutex_release(digivice->mutex);

    if(spawned) notification_message(digivice->notification, &digivice_sequence_virux);
    digivice_changed(digivice, &before);

    if(message.flag) furi_event_flag_set(message.flag, DIGIVICE_DONE_FLAG);
}

static void digivice_deed_callback(const void* message, void* context) {
    Digivice* digivice = context;
    DigiviceMessage event = {
        .type = DigiviceMessageTypeDeed,
        .deed = *(const DolphinDeed*)message,
    };
    // Never block dolphin: a full queue drops the deed.
    furi_message_queue_put(digivice->queue, &event, 0);
}

static void digivice_storage_callback(const void* message, void* context) {
    Digivice* digivice = context;
    const StorageEvent* storage_event = message;
    if(storage_event->type != StorageEventTypeCardMount) return;
    DigiviceMessage event = {.type = DigiviceMessageTypeReload};
    // The tick retries the load if this one is dropped.
    furi_message_queue_put(digivice->queue, &event, 0);
}

static Digivice* digivice_alloc(void) {
    Digivice* digivice = malloc(sizeof(Digivice));
    digivice->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    digivice->pubsub = furi_pubsub_alloc();
    digivice->queue = furi_message_queue_alloc(DIGIVICE_QUEUE_SIZE, sizeof(DigiviceMessage));
    digivice->event_loop = furi_event_loop_alloc();
    digivice->tick_timer = furi_event_loop_timer_alloc(
        digivice->event_loop, digivice_tick, FuriEventLoopTimerTypePeriodic, digivice);
    digivice->flush_timer = furi_event_loop_timer_alloc(
        digivice->event_loop, digivice_flush, FuriEventLoopTimerTypeOnce, digivice);
    digivice->notification = furi_record_open(RECORD_NOTIFICATION);
    digivice->storage = furi_record_open(RECORD_STORAGE);
    digivice_rules_init(&digivice->state, furi_hal_rtc_get_timestamp());
    return digivice;
}

int32_t digivice_srv(void* p) {
    UNUSED(p);

    if(furi_hal_rtc_get_boot_mode() != FuriHalRtcBootModeNormal) {
        FURI_LOG_W(TAG, "Skipping start in special boot mode");
        furi_thread_suspend(furi_thread_get_current_id());
        return 0;
    }

    Digivice* digivice = digivice_alloc();
    if(storage_sd_status(digivice->storage) == FSE_OK) digivice_load(digivice);
    furi_record_create(RECORD_DIGIVICE, digivice);

    digivice->storage_subscription = furi_pubsub_subscribe(
        storage_get_pubsub(digivice->storage), digivice_storage_callback, digivice);

    Dolphin* dolphin = furi_record_open(RECORD_DOLPHIN);
    digivice->deed_subscription =
        furi_pubsub_subscribe(dolphin_get_deed_pubsub(dolphin), digivice_deed_callback, digivice);
    furi_record_close(RECORD_DOLPHIN);

    furi_event_loop_subscribe_message_queue(
        digivice->event_loop,
        digivice->queue,
        FuriEventLoopEventIn,
        digivice_process_message,
        digivice);

    furi_event_loop_timer_start(digivice->tick_timer, DIGIVICE_TICK_S * 1000UL);
    // Catch up on the time the Flipper was off before the first periodic tick.
    furi_event_loop_pend_callback(digivice->event_loop, digivice_tick, digivice);

    furi_event_loop_run(digivice->event_loop);
    return 0;
}
