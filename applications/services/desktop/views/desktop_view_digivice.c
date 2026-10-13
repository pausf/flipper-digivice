#include "desktop_view_digivice.h"

#include <furi.h>
#include <furi_hal.h>
#include <gui/elements.h>
#include <digivice/digivice.h>
#include <digivice/digivice_config.h>
#include <digivice/digivice_draw.h>
#include <storage/storage.h>
#include <loader/loader.h>

// 5 fps keeps the wake-up rate close to the stock dolphin animations.
#define DESKTOP_DIGIVICE_FRAME_MS 200

struct DesktopDigiviceView {
    View* view;
    Digivice* digivice;
    Storage* storage;
    Loader* loader;
    FuriPubSubSubscription* state_subscription;
    FuriPubSubSubscription* loader_subscription;
    FuriPubSubSubscription* storage_subscription;
    FuriTimer* frame_timer;
    DesktopMainViewCallback callback;
    void* context;
    // The view stays on the GUI under running apps, so the timer must follow both flags.
    volatile bool active;
    volatile bool app_running;
};

typedef struct {
    DigiviceState state;
    uint32_t frame;
    uint32_t now;
    uint32_t evo_frame;
    bool sd_ok;
} DesktopDigiviceViewModel;

static void desktop_digivice_view_draw(Canvas* canvas, void* model) {
    DesktopDigiviceViewModel* m = model;
    if(m->state.evo_from != DigiviceFormNone) {
        digivice_draw_evolution(
            canvas, (DigiviceForm)m->state.evo_from, (DigiviceForm)m->state.form, m->evo_frame);
    } else {
        digivice_draw_home(canvas, &m->state, m->frame, m->now, m->sd_ok);
    }
}

static bool desktop_digivice_view_input(InputEvent* event, void* context) {
    DesktopDigiviceView* digivice_view = context;
    // Only short Right is ours; every other key keeps its stock desktop meaning.
    if(event->key == InputKeyRight && event->type == InputTypeShort) {
        if(digivice_view->callback) {
            digivice_view->callback(
                DesktopAnimationEventInteractAnimation, digivice_view->context);
        }
        return true;
    }
    return false;
}

static void desktop_digivice_view_sync_timer(DesktopDigiviceView* digivice_view) {
    if(digivice_view->active && !digivice_view->app_running) {
        furi_timer_start(digivice_view->frame_timer, furi_ms_to_ticks(DESKTOP_DIGIVICE_FRAME_MS));
    } else {
        furi_timer_stop(digivice_view->frame_timer);
    }
}

static void desktop_digivice_view_frame(void* context) {
    DesktopDigiviceView* digivice_view = context;
    if(!digivice_view->active || digivice_view->app_running) return;
    bool evolution_done = false;
    with_view_model(
        digivice_view->view,
        DesktopDigiviceViewModel * m,
        {
            m->frame++;
            m->now = furi_hal_rtc_get_timestamp();
            if(m->state.evo_from != DigiviceFormNone) {
                // The cutscene is authored at 10 fps, so skip a frame at 5 fps.
                if(m->evo_frame < DIGIVICE_EVO_FRAMES) {
                    m->evo_frame += 2;
                } else {
                    evolution_done = true;
                }
            } else {
                m->evo_frame = 0;
            }
        },
        true);
    if(evolution_done) digivice_ack_evolution(digivice_view->digivice);
}

static void desktop_digivice_view_state_callback(const void* message, void* context) {
    DesktopDigiviceView* digivice_view = context;
    const DigiviceState* state = message;
    with_view_model(
        digivice_view->view, DesktopDigiviceViewModel * m, {
            if(m->state.evo_from != state->evo_from) m->evo_frame = 0;
            m->state = *state;
        },
        true);
}

static void desktop_digivice_view_storage_callback(const void* message, void* context) {
    DesktopDigiviceView* digivice_view = context;
    const StorageEvent* event = message;
    if(event->type != StorageEventTypeCardMount && event->type != StorageEventTypeCardUnmount &&
       event->type != StorageEventTypeCardMountError) {
        return;
    }
    bool sd_ok = event->type == StorageEventTypeCardMount;
    with_view_model(
        digivice_view->view, DesktopDigiviceViewModel * m, { m->sd_ok = sd_ok; }, true);
}

static void desktop_digivice_view_loader_callback(const void* message, void* context) {
    DesktopDigiviceView* digivice_view = context;
    const LoaderEvent* event = message;
    if(event->type == LoaderEventTypeApplicationBeforeLoad) {
        digivice_view->app_running = true;
    } else if(
        event->type == LoaderEventTypeApplicationStopped ||
        event->type == LoaderEventTypeApplicationLoadFailed) {
        digivice_view->app_running = false;
    } else {
        return;
    }
    desktop_digivice_view_sync_timer(digivice_view);
}

static void desktop_digivice_view_enter(void* context) {
    DesktopDigiviceView* digivice_view = context;
    digivice_view->active = true;
    desktop_digivice_view_sync_timer(digivice_view);
}

static void desktop_digivice_view_exit(void* context) {
    DesktopDigiviceView* digivice_view = context;
    digivice_view->active = false;
    desktop_digivice_view_sync_timer(digivice_view);
}

DesktopDigiviceView* desktop_digivice_view_alloc(void) {
    DesktopDigiviceView* digivice_view = malloc(sizeof(DesktopDigiviceView));
    digivice_view->view = view_alloc();
    view_allocate_model(
        digivice_view->view, ViewModelTypeLocking, sizeof(DesktopDigiviceViewModel));
    view_set_context(digivice_view->view, digivice_view);
    view_set_draw_callback(digivice_view->view, desktop_digivice_view_draw);
    view_set_input_callback(digivice_view->view, desktop_digivice_view_input);
    view_set_enter_callback(digivice_view->view, desktop_digivice_view_enter);
    view_set_exit_callback(digivice_view->view, desktop_digivice_view_exit);

    digivice_view->frame_timer =
        furi_timer_alloc(desktop_digivice_view_frame, FuriTimerTypePeriodic, digivice_view);

    digivice_view->digivice = furi_record_open(RECORD_DIGIVICE);
    digivice_view->storage = furi_record_open(RECORD_STORAGE);

    digivice_view->loader = furi_record_open(RECORD_LOADER);
    digivice_view->app_running = loader_is_locked(digivice_view->loader);
    digivice_view->loader_subscription = furi_pubsub_subscribe(
        loader_get_pubsub(digivice_view->loader),
        desktop_digivice_view_loader_callback,
        digivice_view);
    digivice_view->storage_subscription = furi_pubsub_subscribe(
        storage_get_pubsub(digivice_view->storage),
        desktop_digivice_view_storage_callback,
        digivice_view);
    // Subscribe before reading so no state publish can fall between the two.
    digivice_view->state_subscription = furi_pubsub_subscribe(
        digivice_get_pubsub(digivice_view->digivice),
        desktop_digivice_view_state_callback,
        digivice_view);

    DigiviceState state;
    digivice_get_state(digivice_view->digivice, &state);
    bool sd_ok = storage_sd_status(digivice_view->storage) == FSE_OK;
    with_view_model(
        digivice_view->view,
        DesktopDigiviceViewModel * m,
        {
            m->state = state;
            m->sd_ok = sd_ok;
            m->now = furi_hal_rtc_get_timestamp();
        },
        false);

    return digivice_view;
}

void desktop_digivice_view_free(DesktopDigiviceView* digivice_view) {
    furi_assert(digivice_view);
    furi_pubsub_unsubscribe(
        loader_get_pubsub(digivice_view->loader), digivice_view->loader_subscription);
    furi_pubsub_unsubscribe(
        storage_get_pubsub(digivice_view->storage), digivice_view->storage_subscription);
    furi_pubsub_unsubscribe(
        digivice_get_pubsub(digivice_view->digivice), digivice_view->state_subscription);
    furi_timer_free(digivice_view->frame_timer);
    furi_record_close(RECORD_LOADER);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_DIGIVICE);
    view_free(digivice_view->view);
    free(digivice_view);
}

View* desktop_digivice_view_get_view(DesktopDigiviceView* digivice_view) {
    furi_assert(digivice_view);
    return digivice_view->view;
}

void desktop_digivice_view_set_callback(
    DesktopDigiviceView* digivice_view,
    DesktopMainViewCallback callback,
    void* context) {
    furi_assert(digivice_view);
    digivice_view->callback = callback;
    digivice_view->context = context;
}
