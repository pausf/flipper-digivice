#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification.h>
#include <digivice/digivice.h>
#include <digivice/digivice_config.h>
#include <digivice/digivice_draw.h>
#include <digivice/digivice_homage.h>
#include <digivice/digivice_i18n.h>
#include <digivice/digivice_roster.h>

#define DIGIVICE_APP_FRAME_MS   100
#define DIGIVICE_APP_AIM_FRAMES 50

// Middle band between the two icon bars.
#define BAND_TOP    12
#define BAND_BOTTOM 52

typedef enum {
    DigiviceScreenMain,
    DigiviceScreenFeed,
    DigiviceScreenTrain,
    DigiviceScreenExpedition,
    DigiviceScreenMap,
    DigiviceScreenHike,
    DigiviceScreenMidway,
    DigiviceScreenGate,
    DigiviceScreenEncounter,
    DigiviceScreenBattle,
    DigiviceScreenProfile,
    DigiviceScreenSettings,
    DigiviceScreenDex,
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
    DigiviceBattlePhaseCharge,
    DigiviceBattlePhaseAim,
    DigiviceBattlePhasePlayerShot,
    DigiviceBattlePhaseFoeShot,
    DigiviceBattlePhaseEnd,
} DigiviceBattlePhase;

/** Ping-pong cursor over DIGIVICE_METER_SEGMENTS for the training minigame. */
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
    uint8_t dex_sel; /**< bestiary preview: form shown */
    DigiviceStr notice;
    uint32_t notice_frames;

    DigiviceCare care;
    uint32_t care_frame;
    uint8_t care_poop; /**< piles on screen before the clean wave wipes them */

    uint8_t feed_from; /**< meter value before the meal, so the hearts can fill up */
    uint8_t feed_food; /**< DigiviceFood on the picker or on the plate */
    uint8_t feed_phase; /**< 0 pick, 1 eat, 2 refused */

    uint8_t setting;
    uint8_t confirm; /**< 0 = none, 1 = "SURE?", 2 = "REALLY?" */

    DigiviceMeter meter;
    bool train_done;
    bool train_success;

    DigiviceScene* scene; /**< area background on the heap, freed when leaving the trail */

    DigiviceBattle battle;
    DigiviceBattlePhase phase;
    uint8_t enc_kind; /**< DigiviceEncounterKind of the current fight */
    uint8_t enc_area; /**< area the fight started in */
    uint8_t zone_start;
    uint8_t zone_w;
    uint8_t cursor; /**< charge meter cursor, frozen once locked */
    bool charge_locked;
    uint8_t charge; /**< DigiviceCharge of the player's shot */
    uint8_t guard0; /**< guard height at aim frame 0 */
    uint8_t aim; /**< DigiviceAim of the player's shot, or the foe's height on its turn */
    bool aim_locked;
    uint32_t aim_frame; /**< aim-phase frame of the press: the guard is sampled 5 frames later */
    uint8_t shot; /**< DigiviceShot on screen */
    uint8_t result; /**< DigiviceBattleResult once decided */
    DigiviceReward reward;
    bool streak_lost;
    bool area_cleared;

    uint32_t trail_scroll; /**< background scroll, advancing only while the pet walks */
    uint8_t map_sel;
    uint32_t hike_start; /**< furi tick when the Hike session started */
    uint32_t hike_paid; /**< one bit per beat that already paid */
    uint8_t hike_good;
    uint8_t hike_left; /**< hourly budget when the session started */
    uint8_t hike_granted;
    uint8_t hike_flash; /**< frames left of the good-tap flash */
    bool hike_done;
    DigiviceMidwayResult midway;
    bool midway_claimed;
    bool midway_accepted;
    // The encounter the app already auto-started or saw declined, so it never insists.
    uint8_t offered_kind;
    uint32_t offered_expires;
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

void digivice_dex_enter(DigiviceApp* app);
void digivice_dex_input(DigiviceApp* app, const InputEvent* event);
void digivice_dex_draw(Canvas* canvas, DigiviceApp* app);

void digivice_settings_enter(DigiviceApp* app);
void digivice_settings_input(DigiviceApp* app, const InputEvent* event);
void digivice_settings_draw(Canvas* canvas, DigiviceApp* app);

void digivice_train_enter(DigiviceApp* app);
void digivice_train_input(DigiviceApp* app, const InputEvent* event);
void digivice_train_tick(DigiviceApp* app);
void digivice_train_draw(Canvas* canvas, DigiviceApp* app);

/** The cached background of the pet's area, allocated on first use. */
const DigiviceScene* digivice_app_scene(DigiviceApp* app);
/** Minutes since midnight from the RTC, for the day and night sky. */
uint16_t digivice_app_day_minute(void);

void digivice_expedition_enter(DigiviceApp* app);
/** Jumps straight to a pending encounter once, when the pet can fight it. */
void digivice_expedition_auto_start(DigiviceApp* app);
void digivice_expedition_input(DigiviceApp* app, const InputEvent* event);
void digivice_expedition_tick(DigiviceApp* app);
void digivice_expedition_draw(Canvas* canvas, DigiviceApp* app);

/** Starts the encounter transition, or shows why the pet cannot fight now. */
void digivice_encounter_start(DigiviceApp* app, DigiviceEncounterKind kind);
void digivice_encounter_input(DigiviceApp* app, const InputEvent* event);
void digivice_encounter_tick(DigiviceApp* app);
void digivice_encounter_draw(Canvas* canvas, DigiviceApp* app);

void digivice_battle_input(DigiviceApp* app, const InputEvent* event);
void digivice_battle_tick(DigiviceApp* app);
void digivice_battle_draw(Canvas* canvas, DigiviceApp* app);
