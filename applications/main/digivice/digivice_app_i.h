#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification.h>
#include <digivice/digivice.h>
#include <digivice/digivice_config.h>
#include <digivice/digivice_draw.h>
#include <digivice/digivice_i18n.h>

#define DIGIVICE_APP_FRAME_MS   100
#define DIGIVICE_APP_AIM_FRAMES 50
#define DIGIVICE_APP_HIT_FRAMES 10

typedef enum {
    DigiviceScreenMain,
    DigiviceScreenFeed,
    DigiviceScreenTrain,
    DigiviceScreenBattle,
    DigiviceScreenProfile,
    DigiviceScreenSettings,
    DigiviceScreenEvolution,
    DigiviceScreenError,
} DigiviceScreen;

/** Care animation playing over the main screen. */
typedef enum {
    DigiviceCareNone,
    DigiviceCareClean,
    DigiviceCareLightOff,
    DigiviceCareLightOn,
    DigiviceCareCure,
} DigiviceCare;

typedef enum {
    DigiviceSettingLanguage,
    DigiviceSettingMode,
    DigiviceSettingReset,
    DigiviceSettingCount,
} DigiviceSetting;

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

    uint8_t menu_index; /**< DigiviceIcon */
    DigiviceStr notice;
    uint32_t notice_frames;

    DigiviceCare care;
    uint32_t care_frame;
    uint8_t care_poop; /**< piles on screen before the clean wave wipes them */

    uint8_t feed_from; /**< hunger before the meal, so the hearts can fill up */

    uint8_t setting;
    uint8_t confirm; /**< 0 = none, 1 = "SURE?", 2 = "REALLY?" */

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
/** A string in the pet's language. */
const char* digivice_app_str(const DigiviceApp* app, DigiviceStr id);
/** Shows `id` in the notice box for DIGIVICE_NOTICE_FRAMES. */
void digivice_app_notice(DigiviceApp* app, DigiviceStr id);
/** Notice for a refused action; DigiviceResultOk shows nothing. */
void digivice_app_refusal(DigiviceApp* app, DigiviceResult result);
/** Title in the top-left corner with the HUD underline. */
void digivice_app_draw_title(Canvas* canvas, const char* title);

void digivice_meter_start(DigiviceMeter* meter);
void digivice_meter_step(DigiviceMeter* meter);
bool digivice_meter_hit(const DigiviceMeter* meter);

void digivice_feed_enter(DigiviceApp* app);
void digivice_feed_input(DigiviceApp* app, const InputEvent* event);
void digivice_feed_tick(DigiviceApp* app);
void digivice_feed_draw(Canvas* canvas, DigiviceApp* app);

void digivice_care_start(DigiviceApp* app, DigiviceCare care);
void digivice_care_tick(DigiviceApp* app);
/** Draws the running care animation over the main screen's middle band. */
void digivice_care_draw(Canvas* canvas, DigiviceApp* app, int32_t pet_x);

void digivice_profile_draw(Canvas* canvas, DigiviceApp* app);

void digivice_settings_enter(DigiviceApp* app);
void digivice_settings_input(DigiviceApp* app, const InputEvent* event);
void digivice_settings_draw(Canvas* canvas, DigiviceApp* app);

void digivice_train_enter(DigiviceApp* app);
void digivice_train_input(DigiviceApp* app, const InputEvent* event);
void digivice_train_tick(DigiviceApp* app);
void digivice_train_draw(Canvas* canvas, DigiviceApp* app);

void digivice_battle_enter(DigiviceApp* app);
void digivice_battle_input(DigiviceApp* app, const InputEvent* event);
void digivice_battle_tick(DigiviceApp* app);
void digivice_battle_draw(Canvas* canvas, DigiviceApp* app);
