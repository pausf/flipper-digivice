#pragma once

// Pure game rules: no furi includes, so the host test can compile this file with plain cc.

#include <stdbool.h>
#include <stdint.h>

#include <dolphin/helpers/dolphin_deed.h>

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

DigiviceStage digivice_rules_stage(const DigiviceState* s);
DigiviceType digivice_rules_player_type(const DigiviceState* s);
DigiviceType digivice_rules_deed_type(DolphinDeed deed);
bool digivice_rules_is_spawn_deed(DolphinDeed deed);
bool digivice_rules_is_sleep_hour(uint8_t hour);
uint32_t digivice_rules_awake_seconds(uint32_t from, uint32_t to);

/** Applies time-based rules from s->last_tick to now. */
void digivice_rules_tick(DigiviceState* s, uint32_t now);

/** Grants EXP and data for a deed and rolls a VIRUX spawn (rng_pct 0..99). True on spawn. */
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
void digivice_rules_train(DigiviceState* s, bool success, uint32_t now);
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
