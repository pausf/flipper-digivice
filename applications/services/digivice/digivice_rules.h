#pragma once

// Pure game rules: no furi includes, so the host test can compile this file with plain cc.

#include <stdbool.h>
#include <stddef.h>
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
    DigiviceFormPipo,
    DigiviceFormChispi,
    DigiviceFormOrejin,
    DigiviceFormColix,
    DigiviceFormKorno,
    DigiviceFormChipko,
    DigiviceFormEstatik,
    DigiviceFormMokobit,
    DigiviceFormAriete,
    DigiviceFormEcolobo,
    DigiviceFormVampixel,
    DigiviceFormCerrojo,
    DigiviceFormBuhoscan,
    DigiviceFormFuzzcan,
    DigiviceFormSilbapo,
    DigiviceFormChatarro,
    DigiviceFormCharco,
    DigiviceFormLanzero,
    DigiviceFormWyvolt,
    DigiviceFormFangorey,
    DigiviceFormBovedax,
    DigiviceFormKoloso,
    DigiviceFormAureola,
    DigiviceFormDrakstat,
    DigiviceFormOrakulo,
    DigiviceFormNularca,
    DigiviceFormPrismar,
    DigiviceFormBaliza,
    DigiviceFormCount,
    DigiviceFormNone = 0xFF,
} DigiviceForm;

typedef enum {
    DigiviceStageEgg,
    DigiviceStageBaby, /**< Baby I */
    DigiviceStageBaby2,
    DigiviceStageRookie, /**< Novato */
    DigiviceStageChampion,
    DigiviceStageUltimate, /**< Supremo */
    DigiviceStageMega,
    DigiviceStageCount,
} DigiviceStage;

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
    DigiviceResultTooYoung, /**< an Egg cannot fight */
} DigiviceResult;

typedef enum {
    DigiviceItemNone,
    DigiviceItemShield, /**< cancels the first foe hit of the next battle */
    DigiviceItemCount,
} DigiviceItem;

typedef enum {
    DigiviceFoodMeat, /**< hunger +1 */
    DigiviceFoodProtein, /**< vigor +1 and strength */
    DigiviceFoodCount,
} DigiviceFood;

/** Battle attributes. Triangle: Portador > Ruido > Cifrado > Portador; Nulo is neutral. */
typedef enum {
    DigiviceAttrPortador,
    DigiviceAttrRuido,
    DigiviceAttrCifrado,
    DigiviceAttrNulo,
    DigiviceAttrCount,
} DigiviceAttr;

/** `enc_foe` value when no trail encounter is pending. */
#define DIGIVICE_ENC_NONE 0xFF

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

/** Exact copy of the v2 save layout (saved_struct version 0x02). Never change it. */
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
} DigiviceStateV2;

/** Exact copy of the v3 save layout (saved_struct version 0x03). Never change it. */
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
    // v3 fields, appended so the v2 prefix keeps its offsets.
    uint8_t area; /**< 0..DIGIVICE_AREA_COUNT-1, the active Expedition area */
    uint8_t cleared; /**< bit i set once the boss of area i is defeated */
    uint8_t enc_foe; /**< pending trail foe, index 0 or 1 in the area pool, DIGIVICE_ENC_NONE */
    uint8_t mid_state; /**< midway event: 0 not reached, 1 available, 2 done */
    uint8_t item; /**< DigiviceItem */
    uint8_t hike_count; /**< Hike paces granted in the current bonus window */
    uint8_t scan_count; /**< scan paces granted in the current bonus window */
    uint8_t reserved3;
    uint16_t distance; /**< paces left in the area; 0 means the boss is ready */
    uint16_t paces_since_fight; /**< paces since the last encounter spawned, saturates */
    uint32_t pace_acc_s; /**< awake seconds toward the next pace */
    uint32_t enc_expires; /**< when the pending trail encounter leaves, 0 when none */
    uint32_t bonus_window_start; /**< start of the hourly Hike and scan window, 0 when none */
    uint32_t injured_until; /**< end of the defeat injury, 0 when healthy */
    uint32_t rng; /**< xorshift32 state, never 0 */
} DigiviceStateV3;

#define DIGIVICE_CALL_HUNGER     0x01
#define DIGIVICE_CALL_VIGOR      0x02
#define DIGIVICE_FEED_ARM_HUNGER 0x01
#define DIGIVICE_FEED_ARM_VIGOR  0x02

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
    // v3 fields, appended so the v2 prefix keeps its offsets.
    uint8_t area; /**< 0..DIGIVICE_AREA_COUNT-1, the active Expedition area */
    uint8_t cleared; /**< bit i set once the boss of area i is defeated */
    uint8_t enc_foe; /**< pending trail foe, index 0 or 1 in the area pool, DIGIVICE_ENC_NONE */
    uint8_t mid_state; /**< midway event: 0 not reached, 1 available, 2 done */
    uint8_t item; /**< DigiviceItem */
    uint8_t hike_count; /**< Hike paces granted in the current bonus window */
    uint8_t scan_count; /**< scan paces granted in the current bonus window */
    uint8_t reserved3;
    uint16_t distance; /**< paces left in the area; 0 means the boss is ready */
    uint16_t paces_since_fight; /**< paces since the last encounter spawned, saturates */
    uint32_t pace_acc_s; /**< awake seconds toward the next pace */
    uint32_t enc_expires; /**< when the pending trail encounter leaves, 0 when none */
    uint32_t bonus_window_start; /**< start of the hourly Hike and scan window, 0 when none */
    uint32_t injured_until; /**< end of the defeat injury, 0 when healthy */
    uint32_t rng; /**< xorshift32 state, never 0 */
    // v4 fields, appended so the v3 prefix keeps its offsets.
    uint8_t vigor; /**< 0..DIGIVICE_VIGOR_MAX, the strength hearts protein fills */
    uint8_t call_flags; /**< DIGIVICE_CALL_HUNGER | DIGIVICE_CALL_VIGOR: calls waiting */
    uint8_t feed_flags; /**< DIGIVICE_FEED_ARM_*: a refusal of that food counts as overfeed */
    uint8_t evo_mistakes; /**< call mistakes in this stage, reset on evolution */
    uint8_t train_count; /**< successful trainings in this stage */
    uint8_t battles; /**< Win or Lose results in this stage */
    uint8_t wins_stage; /**< Wins in this stage */
    uint8_t overfeed; /**< refusals counted in this stage */
    uint8_t night_owl; /**< sleeps with the light left on, in this stage */
    uint8_t reserved4[3];
    uint16_t call_hunger_s; /**< awake seconds the hunger call has waited */
    uint16_t call_vigor_s; /**< awake seconds the vigor call has waited */
    uint32_t stage_since; /**< when the current form began, wall clock */
    uint32_t vigor_acc_s; /**< awake seconds toward the next vigor heart lost */
} DigiviceState;

_Static_assert(sizeof(DigiviceStateV1) == 56, "the v1 file layout is frozen");
_Static_assert(sizeof(DigiviceStateV2) == 84, "the v2 file layout is frozen");
_Static_assert(
    offsetof(DigiviceState, area) == sizeof(DigiviceStateV2),
    "v3 keeps the v2 prefix");
_Static_assert(sizeof(DigiviceStateV3) == 116, "the v3 file layout is frozen");
_Static_assert(
    offsetof(DigiviceState, vigor) == sizeof(DigiviceStateV3),
    "v4 keeps the v3 prefix");
_Static_assert(sizeof(DigiviceState) == 140, "v4 file layout");

void digivice_rules_init(DigiviceState* s, uint32_t now);
/** Fresh egg that keeps the owner's language and mode. */
void digivice_rules_reset(DigiviceState* s, uint32_t now);
/** Copies every v1 field and gives the v2 ones their defaults (Spanish, Normal, light on). */
void digivice_rules_migrate_v1(const DigiviceStateV1* v1, DigiviceStateV2* v2);
/** Copies the v2 prefix and yields the current state: v3 defaults (area 1 from its start), then
 * v4 defaults. */
void digivice_rules_migrate_v2(const DigiviceStateV2* v2, DigiviceState* s);
/** Copies the v3 prefix and gives the v4 fields their defaults. */
void digivice_rules_migrate_v3(const DigiviceStateV3* v3, DigiviceState* s);
/** Repairs out-of-range fields from a loaded file instead of trusting them. */
void digivice_rules_sanitize(DigiviceState* s);
/** Return false (and change nothing) when the value is out of range. */
bool digivice_rules_set_lang(DigiviceState* s, uint8_t lang);
bool digivice_rules_set_mode(DigiviceState* s, uint8_t mode);

/** Paces in an area; an out-of-range area reads as the last one. */
uint16_t digivice_rules_area_length(uint8_t area);

/** xorshift32 step on s->rng. Never returns 0. */
uint32_t digivice_rules_rng(DigiviceState* s);
/** Next random number in 0..99. */
uint8_t digivice_rules_rng_pct(DigiviceState* s);
/** XORs hardware entropy into s->rng and keeps it nonzero. */
void digivice_rules_mix_entropy(DigiviceState* s, uint32_t entropy);

/** +1 when the attacker beats the defender, -1 when the defender beats it, 0 otherwise. */
int8_t digivice_rules_attr_adv(uint8_t attacker, uint8_t defender);
/** Virubyte is Ruido; the hollow early forms and Plugdrake are Nulo. */
DigiviceAttr digivice_rules_form_attr(const DigiviceState* s);
/** VIRUX attribute from its data type: NFC Cifrado, Sub-GHz Portador, IR Ruido, USB Nulo. */
DigiviceAttr digivice_rules_type_attr(DigiviceType type);

DigiviceStage digivice_rules_stage(const DigiviceState* s);
DigiviceType digivice_rules_player_type(const DigiviceState* s);
DigiviceType digivice_rules_deed_type(DolphinDeed deed);
bool digivice_rules_is_spawn_deed(DolphinDeed deed);
bool digivice_rules_is_sleep_hour(uint8_t hour);
bool digivice_rules_is_asleep(uint32_t now);
uint32_t digivice_rules_awake_seconds(uint32_t from, uint32_t to);

/** True once per night: a tick (prev -> now, normal tick spacing) crossed into the sleep window
 * while the light is still on and the pet is past Egg. Call with the state after the tick. */
bool digivice_rules_bedtime_started(const DigiviceState* s, uint32_t prev, uint32_t now);

/** Counts a Win or Lose for the evolution counters (saturating). */
void digivice_rules_count_battle(DigiviceState* s, bool won);

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

/** Advances at most one evolution, by time in stage and the hidden counters. Returns true when
 * the form changed. */
bool digivice_rules_evolve(DigiviceState* s, uint32_t now);

/** Maps the v1 to v3 forms (BITLING, BYTEMON and the four drakes, in form, prev_form and
 * evo_from) onto the v3.2 tree, restarting stage_since at now. True when anything changed. */
bool digivice_rules_upgrade_legacy(DigiviceState* s, uint32_t now);

/** True while a hunger or vigor call is waiting for an answer. */
bool digivice_rules_call_active(const DigiviceState* s);

/** Returns false when the food was refused: that is an overfeed, counted once per refill. */
bool digivice_rules_feed(DigiviceState* s, DigiviceFood food, uint32_t now);
DigiviceResult digivice_rules_can_train(const DigiviceState* s, uint32_t now);
/** Refused while sick, asleep or still an Egg. */
DigiviceResult digivice_rules_can_battle(const DigiviceState* s, uint32_t now);
/** Refused while sick or asleep; otherwise v1 effects plus training EXP. */
DigiviceResult digivice_rules_train(DigiviceState* s, bool success, uint32_t now);
/** Returns false when there is nothing to clean. */
bool digivice_rules_clean(DigiviceState* s, uint32_t now);
/** Uses one dose. Returns false (a refusal) on a healthy pet. */
bool digivice_rules_cure(DigiviceState* s, uint32_t now);
/** Toggles the light while asleep; refused while awake, where the light always stays on. */
DigiviceResult digivice_rules_toggle_light(DigiviceState* s, uint32_t now);
void digivice_rules_ack_evolution(DigiviceState* s);

bool digivice_rules_virux_active(const DigiviceState* s, uint32_t now);
/** Adds (or with a negative delta removes) corruption, with the Virubyte transform at 100. */
void digivice_rules_add_corruption(DigiviceState* s, int32_t delta, uint32_t now);
bool digivice_rules_glitchy(const DigiviceState* s);
bool digivice_rules_attack_misses(const DigiviceState* s, uint8_t rng_pct);
bool digivice_rules_timing_hit(uint8_t cursor, uint8_t zone_start);

/** True when two states differ only in tick bookkeeping, so the service can skip a flash write. */
bool digivice_rules_same_visible(const DigiviceState* a, const DigiviceState* b);

#ifdef __cplusplus
}
#endif
