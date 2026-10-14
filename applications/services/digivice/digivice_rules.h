#pragma once

// Pure game rules: no furi includes, so the host test can compile this file with plain cc.

#include <stdbool.h>
#include <stdint.h>

#include <dolphin/helpers/dolphin_deed.h>

#include "digivice_i18n.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigiviceTypeNfc,
    DigiviceTypeSubGhz,
    DigiviceTypeIr,
    DigiviceTypeUsb,
    DigiviceTypeCount,
    DigiviceTypeNone = 0xFF,
} DigiviceType;

typedef enum {
    DigiviceFormByteegg,
    DigiviceFormBitling,
    DigiviceFormBytemon,
    DigiviceFormTagdrake,
    DigiviceFormWavedrake,
    DigiviceFormBeamdrake,
    DigiviceFormPlugdrake,
    DigiviceFormVirubyte,
    DigiviceFormCount,
    DigiviceFormNone = 0xFF,
} DigiviceForm;

typedef enum {
    DigiviceStageEgg,
    DigiviceStageBaby,
    DigiviceStageRookie,
    DigiviceStageChampion,
    DigiviceStageCount,
} DigiviceStage;

typedef enum {
    DigiviceBattleResultWin,
    DigiviceBattleResultLose,
    DigiviceBattleResultFled,
} DigiviceBattleResult;

typedef enum {
    DigiviceBattleOngoing,
    DigiviceBattleWon,
    DigiviceBattleLost,
} DigiviceBattleOutcome;

typedef enum {
    DigiviceModeNormal, /**< store-bought V-Pet: Flipper deeds are ignored */
    DigiviceModeHacker, /**< v1 play: deeds grant EXP and data and spawn VIRUX */
    DigiviceModeCount,
} DigiviceMode;

/** Outcome of a care action: OK, or why it was refused. */
typedef enum {
    DigiviceResultOk,
    DigiviceResultSick,
    DigiviceResultAsleep,
    DigiviceResultAwake,
    DigiviceResultHealthy,
    DigiviceResultNothingToClean,
} DigiviceResult;

/** Exact copy of the v1 save layout (saved_struct version 0x01). Never change it. */
typedef struct {
    uint8_t form;
    uint8_t prev_form;
    uint8_t evo_from;
    uint8_t hunger;
    uint8_t mood;
    uint8_t corruption;
    uint8_t virux_type;
    uint8_t reserved;
    uint16_t strength;
    uint16_t wins;
    uint16_t losses;
    uint16_t data[DigiviceTypeCount];
    uint32_t exp;
    uint32_t virux_expires;
    uint32_t last_tick;
    uint32_t born_at;
    uint32_t clean_since;
    uint32_t hunger_acc_s;
    uint32_t mood_acc_s;
    uint32_t corrupt_acc_s;
} DigiviceStateV1;

/** Persisted pet state. Fixed-width fields only: it is written as-is by saved_struct. */
typedef struct {
    uint8_t form; /**< DigiviceForm */
    uint8_t prev_form; /**< form to restore after corruption recovery */
    uint8_t evo_from; /**< form before the last evolution, DigiviceFormNone when none is pending */
    uint8_t hunger; /**< 0..DIGIVICE_HUNGER_MAX */
    uint8_t mood; /**< 0..DIGIVICE_MOOD_MAX */
    uint8_t corruption; /**< 0..DIGIVICE_CORRUPTION_MAX */
    uint8_t virux_type; /**< DigiviceType, DigiviceTypeNone when no VIRUX is pending */
    uint8_t reserved;
    uint16_t strength;
    uint16_t wins;
    uint16_t losses;
    uint16_t data[DigiviceTypeCount];
    uint32_t exp;
    uint32_t virux_expires;
    uint32_t last_tick;
    uint32_t born_at;
    uint32_t clean_since; /**< when corruption last reached 0, 0 while corruption > 0 */
    // Awake seconds carried between ticks so 60 s ticks lose nothing to rounding.
    uint32_t hunger_acc_s;
    uint32_t mood_acc_s;
    uint32_t corrupt_acc_s;
    // v2 fields, appended so the v1 prefix keeps its offsets.
    uint8_t lang; /**< DigiviceLang */
    uint8_t mode; /**< DigiviceMode */
    uint8_t poop; /**< 0..DIGIVICE_POOP_MAX */
    uint8_t sick; /**< 0/1 */
    uint8_t care_mistakes; /**< since the last full cure, saturates at 255 */
    uint8_t light_on; /**< 0/1, forced back to 1 whenever the pet is awake */
    uint8_t doses_needed; /**< remaining doses while sick */
    uint8_t reserved2;
    uint32_t poop_acc_s; /**< awake seconds toward the next poop */
    uint32_t poop_dirty_since; /**< when the oldest uncleaned poop appeared, 0 when none */
    uint32_t starving_since; /**< when hunger hit 0, 0 while hunger > 0 */
    uint32_t sick_since; /**< when sickness started, 0 while healthy */
    uint32_t light_on_asleep_s; /**< seconds the light stayed on in the current sleep window */
} DigiviceState;

typedef struct {
    uint16_t player_hp;
    uint16_t player_hp_max;
    uint16_t foe_hp;
    uint16_t foe_hp_max;
    uint8_t player_type;
    uint8_t foe_type;
    uint8_t stage;
    bool defending;
} DigiviceBattle;

void digivice_rules_init(DigiviceState* s, uint32_t now);
/** Fresh egg that keeps the owner's language and mode. */
void digivice_rules_reset(DigiviceState* s, uint32_t now);
/** Copies every v1 field and gives the v2 ones their defaults (Spanish, Normal, light on). */
void digivice_rules_migrate_v1(const DigiviceStateV1* v1, DigiviceState* s);
/** Clamps an out-of-range lang or mode from a loaded file to the default. */
void digivice_rules_sanitize(DigiviceState* s);
/** Return false (and change nothing) when the value is out of range. */
bool digivice_rules_set_lang(DigiviceState* s, uint8_t lang);
bool digivice_rules_set_mode(DigiviceState* s, uint8_t mode);

DigiviceStage digivice_rules_stage(const DigiviceState* s);
DigiviceType digivice_rules_player_type(const DigiviceState* s);
DigiviceType digivice_rules_deed_type(DolphinDeed deed);
bool digivice_rules_is_spawn_deed(DolphinDeed deed);
bool digivice_rules_is_sleep_hour(uint8_t hour);
bool digivice_rules_is_asleep(uint32_t now);
uint32_t digivice_rules_awake_seconds(uint32_t from, uint32_t to);

/** Applies time-based rules from s->last_tick to now. */
void digivice_rules_tick(DigiviceState* s, uint32_t now);

/** Hacker mode only: grants EXP and data for a deed and rolls a VIRUX spawn (rng_pct 0..99).
 * True on spawn. In Normal mode it changes nothing. */
bool digivice_rules_on_deed(
    DigiviceState* s,
    DolphinDeed deed,
    uint8_t weight,
    uint32_t now,
    uint8_t rng_pct);

/** Advances at most one evolution stage. Returns true when the form changed. */
bool digivice_rules_evolve(DigiviceState* s, uint32_t now);

/** Returns false (and changes nothing) when hunger is already full. */
bool digivice_rules_feed(DigiviceState* s, uint32_t now);
DigiviceResult digivice_rules_can_train(const DigiviceState* s, uint32_t now);
DigiviceResult digivice_rules_can_battle(const DigiviceState* s);
/** Refused while sick or asleep; otherwise v1 effects plus training EXP. */
DigiviceResult digivice_rules_train(DigiviceState* s, bool success, uint32_t now);
void digivice_rules_battle_result(DigiviceState* s, DigiviceBattleResult result, uint32_t now);
void digivice_rules_ack_evolution(DigiviceState* s);

bool digivice_rules_virux_active(const DigiviceState* s, uint32_t now);
bool digivice_rules_glitchy(const DigiviceState* s);
bool digivice_rules_attack_misses(const DigiviceState* s, uint8_t rng_pct);
bool digivice_rules_timing_hit(uint8_t cursor, uint8_t zone_start);

/** True when two states differ only in tick bookkeeping, so the service can skip a flash write. */
bool digivice_rules_same_visible(const DigiviceState* a, const DigiviceState* b);

uint16_t digivice_rules_type_x10(DigiviceType attacker, DigiviceType defender);
uint16_t digivice_rules_damage(
    uint16_t base,
    uint16_t strength,
    bool in_zone,
    DigiviceType attacker,
    DigiviceType defender);

void digivice_battle_init(DigiviceBattle* b, const DigiviceState* s);
/** Returns the damage dealt, 0 on a corruption miss. */
uint16_t digivice_battle_player_attack(
    DigiviceBattle* b,
    const DigiviceState* s,
    bool in_zone,
    uint8_t rng_pct);
void digivice_battle_defend(DigiviceBattle* b);
bool digivice_battle_flee(uint8_t rng_pct);
uint16_t digivice_battle_foe_attack(DigiviceBattle* b);
DigiviceBattleOutcome digivice_battle_outcome(const DigiviceBattle* b);

#ifdef __cplusplus
}
#endif
