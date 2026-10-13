#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification.h>
#include <digivice/digivice.h>
#include <digivice/digivice_config.h>
#include <digivice/digivice_draw.h>

#define DIGIVICE_APP_FRAME_MS   100
#define DIGIVICE_APP_AIM_FRAMES 50
#define DIGIVICE_APP_HIT_FRAMES 10

typedef enum {
    DigiviceScreenMenu,
    DigiviceScreenStats,
    DigiviceScreenTrain,
    DigiviceScreenBattle,
    DigiviceScreenEvolution,
    DigiviceScreenError,
} DigiviceScreen;

typedef enum {
    DigiviceMenuFeed,
    DigiviceMenuTrain,
    DigiviceMenuStats,
    DigiviceMenuBattle,
    DigiviceMenuCount,
} DigiviceMenuItem;

typedef enum {
    DigiviceBattlePhaseIntro,
    DigiviceBattlePhaseChoose,
    DigiviceBattlePhaseAim,
    DigiviceBattlePhasePlayerHit,
    DigiviceBattlePhaseFoeHit,
    DigiviceBattlePhaseEnd,
} DigiviceBattlePhase;

typedef enum {
    DigiviceBattleChoiceAttack,
    DigiviceBattleChoiceDefend,
    DigiviceBattleChoiceFlee,
    DigiviceBattleChoiceCount,
} DigiviceBattleChoice;

/** Ping-pong cursor over DIGIVICE_METER_SEGMENTS, shared by training and battle. */
typedef struct {
    uint8_t zone_start;
    uint8_t cursor;
    int8_t direction;
} DigiviceMeter;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* input_queue;
    FuriMutex* mutex;
    Digivice* digivice;
    NotificationApp* notification;

    DigiviceState state;
    DigiviceScreen screen;
    uint32_t frame;
    uint32_t screen_frame;
    bool running;

    uint8_t menu_index;
    const char* toast;
    uint32_t toast_frames;

    DigiviceMeter meter;
    bool train_done;
    bool train_success;

    DigiviceBattle battle;
    DigiviceBattlePhase phase;
    uint8_t choice;
    uint16_t last_damage;
    DigiviceBattleResult result;
    const char* message;
} DigiviceApp;

void digivice_app_set_screen(DigiviceApp* app, DigiviceScreen screen);
/** Re-reads the service state and jumps to the cutscene when an evolution is pending. */
void digivice_app_refresh(DigiviceApp* app);
uint8_t digivice_app_rng_pct(void);

void digivice_meter_start(DigiviceMeter* meter);
void digivice_meter_step(DigiviceMeter* meter);
bool digivice_meter_hit(const DigiviceMeter* meter);

void digivice_train_enter(DigiviceApp* app);
void digivice_train_input(DigiviceApp* app, const InputEvent* event);
void digivice_train_tick(DigiviceApp* app);
void digivice_train_draw(Canvas* canvas, DigiviceApp* app);

void digivice_battle_enter(DigiviceApp* app);
void digivice_battle_input(DigiviceApp* app, const InputEvent* event);
void digivice_battle_tick(DigiviceApp* app);
void digivice_battle_draw(Canvas* canvas, DigiviceApp* app);
