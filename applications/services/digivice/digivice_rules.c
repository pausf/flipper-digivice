#include "digivice_rules.h"
#include "digivice_config.h"

#include <string.h>

static uint32_t min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

static void add_u16(uint16_t* value, uint32_t amount) {
    uint32_t sum = (uint32_t)*value + amount;
    *value = sum > UINT16_MAX ? UINT16_MAX : (uint16_t)sum;
}

static void evolve_to(DigiviceState* s, uint8_t form) {
    s->evo_from = s->form;
    s->form = form;
}

static void set_corruption(DigiviceState* s, int32_t value, uint32_t now) {
    if(value < 0) value = 0;
    if(value > DIGIVICE_CORRUPTION_MAX) value = DIGIVICE_CORRUPTION_MAX;
    s->corruption = (uint8_t)value;

    if(value > 0) {
        s->clean_since = 0;
    } else if(s->clean_since == 0) {
        s->clean_since = now;
    }

    if(value == DIGIVICE_CORRUPTION_MAX && s->form != DigiviceFormVirubyte &&
       digivice_rules_stage(s) >= DigiviceStageRookie) {
        s->prev_form = s->form;
        evolve_to(s, DigiviceFormVirubyte);
    }
}

void digivice_rules_init(DigiviceState* s, uint32_t now) {
    memset(s, 0, sizeof(DigiviceState));
    s->form = DigiviceFormByteegg;
    s->prev_form = DigiviceFormByteegg;
    s->evo_from = DigiviceFormNone;
    s->hunger = DIGIVICE_HUNGER_MAX;
    s->mood = DIGIVICE_MOOD_MAX;
    s->virux_type = DigiviceTypeNone;
    s->last_tick = now;
    s->born_at = now;
    s->clean_since = now;
}

DigiviceStage digivice_rules_stage(const DigiviceState* s) {
    uint8_t form = s->form == DigiviceFormVirubyte ? s->prev_form : s->form;
    switch(form) {
    case DigiviceFormByteegg:
        return DigiviceStageEgg;
    case DigiviceFormBitling:
        return DigiviceStageBaby;
    case DigiviceFormBytemon:
        return DigiviceStageRookie;
    default:
        return DigiviceStageChampion;
    }
}

bool digivice_rules_is_sleep_hour(uint8_t hour) {
    return hour >= DIGIVICE_SLEEP_START_HOUR || hour < DIGIVICE_SLEEP_END_HOUR;
}

uint32_t digivice_rules_awake_seconds(uint32_t from, uint32_t to) {
    uint32_t awake = 0;
    uint32_t t = from;
    while(t < to) {
        uint32_t next = min_u32((t / 3600UL + 1UL) * 3600UL, to);
        uint8_t hour = (uint8_t)((t % 86400UL) / 3600UL);
        if(!digivice_rules_is_sleep_hour(hour)) awake += next - t;
        t = next;
    }
    return awake;
}

void digivice_rules_tick(DigiviceState* s, uint32_t now) {
    uint32_t elapsed = now > s->last_tick ? now - s->last_tick : 0;
    elapsed = min_u32(elapsed, DIGIVICE_ELAPSED_MAX_S);
    uint32_t awake = digivice_rules_awake_seconds(now - elapsed, now);

    // Advance event by event so corruption only accrues while hunger or mood is really zero.
    while(awake > 0) {
        uint32_t step = awake;
        if(s->hunger > 0) step = min_u32(step, DIGIVICE_HUNGER_DECAY_S - s->hunger_acc_s);
        if(s->mood > 0) step = min_u32(step, DIGIVICE_MOOD_DECAY_S - s->mood_acc_s);

        if(s->hunger > 0) s->hunger_acc_s += step;
        if(s->mood > 0) s->mood_acc_s += step;
        if(s->hunger == 0 || s->mood == 0) s->corrupt_acc_s += step;
        awake -= step;

        if(s->hunger_acc_s >= DIGIVICE_HUNGER_DECAY_S) {
            s->hunger_acc_s = 0;
            s->hunger--;
        }
        if(s->mood_acc_s >= DIGIVICE_MOOD_DECAY_S) {
            s->mood_acc_s = 0;
            s->mood--;
        }
        if(s->corrupt_acc_s >= 3600UL) {
            uint32_t hours = s->corrupt_acc_s / 3600UL;
            s->corrupt_acc_s %= 3600UL;
            set_corruption(
                s, (int32_t)s->corruption + (int32_t)(hours * DIGIVICE_CORRUPTION_PER_HOUR), now);
        }
    }

    if(s->virux_type != DigiviceTypeNone && now >= s->virux_expires) {
        s->virux_type = DigiviceTypeNone;
    }

    if(s->form == DigiviceFormVirubyte && s->corruption == 0 && s->clean_since != 0 &&
       now >= s->clean_since && now - s->clean_since >= DIGIVICE_CORRUPTION_RECOVERY_S) {
        evolve_to(s, s->prev_form);
    }

    s->last_tick = now;
}

bool digivice_rules_feed(DigiviceState* s, uint32_t now) {
    if(s->hunger >= DIGIVICE_HUNGER_MAX) return false;
    s->hunger++;
    set_corruption(s, (int32_t)s->corruption - DIGIVICE_CORRUPTION_FEED_DROP, now);
    return true;
}

void digivice_rules_train(DigiviceState* s, bool success, uint32_t now) {
    if(s->hunger > 0) s->hunger--;
    if(!success) return;
    add_u16(&s->strength, 1);
    if(s->mood < DIGIVICE_MOOD_MAX) s->mood++;
    set_corruption(s, (int32_t)s->corruption - DIGIVICE_CORRUPTION_TRAIN_DROP, now);
}

bool digivice_rules_glitchy(const DigiviceState* s) {
    return s->corruption >= DIGIVICE_CORRUPTION_GLITCH;
}

bool digivice_rules_attack_misses(const DigiviceState* s, uint8_t rng_pct) {
    return digivice_rules_glitchy(s) && rng_pct < DIGIVICE_CORRUPTION_MISS_PCT;
}

bool digivice_rules_timing_hit(uint8_t cursor, uint8_t zone_start) {
    return cursor >= zone_start && cursor < zone_start + DIGIVICE_METER_ZONE;
}

bool digivice_rules_same_visible(const DigiviceState* a, const DigiviceState* b) {
    DigiviceState x = *a;
    DigiviceState y = *b;
    x.last_tick = y.last_tick = 0;
    x.hunger_acc_s = y.hunger_acc_s = 0;
    x.mood_acc_s = y.mood_acc_s = 0;
    x.corrupt_acc_s = y.corrupt_acc_s = 0;
    return memcmp(&x, &y, sizeof(DigiviceState)) == 0;
}

static const uint8_t champion_of[DigiviceTypeCount] = {
    [DigiviceTypeNfc] = DigiviceFormTagdrake,
    [DigiviceTypeSubGhz] = DigiviceFormWavedrake,
    [DigiviceTypeIr] = DigiviceFormBeamdrake,
    [DigiviceTypeUsb] = DigiviceFormPlugdrake,
};

DigiviceType digivice_rules_player_type(const DigiviceState* s) {
    for(uint8_t t = 0; t < DigiviceTypeCount; t++) {
        if(champion_of[t] == s->form) return (DigiviceType)t;
    }
    return DigiviceTypeNone;
}

DigiviceType digivice_rules_deed_type(DolphinDeed deed) {
    switch(deed) {
    case DolphinDeedSubGhzReceiverInfo:
    case DolphinDeedSubGhzSave:
    case DolphinDeedSubGhzRawRec:
    case DolphinDeedSubGhzAddManually:
    case DolphinDeedSubGhzSend:
    case DolphinDeedSubGhzFrequencyAnalyzer:
        return DigiviceTypeSubGhz;

    case DolphinDeedRfidRead:
    case DolphinDeedRfidReadSuccess:
    case DolphinDeedRfidSave:
    case DolphinDeedRfidEmulate:
    case DolphinDeedRfidAdd:
    case DolphinDeedNfcRead:
    case DolphinDeedNfcReadSuccess:
    case DolphinDeedNfcSave:
    case DolphinDeedNfcDetectReader:
    case DolphinDeedNfcEmulate:
    case DolphinDeedNfcKeyAdd:
    case DolphinDeedNfcAddSave:
    case DolphinDeedNfcAddEmulate:
    case DolphinDeedIbuttonRead:
    case DolphinDeedIbuttonReadSuccess:
    case DolphinDeedIbuttonSave:
    case DolphinDeedIbuttonEmulate:
    case DolphinDeedIbuttonAdd:
        return DigiviceTypeNfc;

    case DolphinDeedIrSend:
    case DolphinDeedIrLearnSuccess:
    case DolphinDeedIrSave:
        return DigiviceTypeIr;

    case DolphinDeedBadUsbPlayScript:
    case DolphinDeedU2fAuthorized:
    case DolphinDeedGpioUartBridge:
        return DigiviceTypeUsb;

    default:
        return DigiviceTypeNone;
    }
}

bool digivice_rules_is_spawn_deed(DolphinDeed deed) {
    return deed == DolphinDeedNfcRead || deed == DolphinDeedRfidRead ||
           deed == DolphinDeedIbuttonRead || deed == DolphinDeedSubGhzReceiverInfo ||
           deed == DolphinDeedSubGhzFrequencyAnalyzer;
}

bool digivice_rules_virux_active(const DigiviceState* s, uint32_t now) {
    return s->virux_type != DigiviceTypeNone && now < s->virux_expires;
}

bool digivice_rules_on_deed(
    DigiviceState* s,
    DolphinDeed deed,
    uint8_t weight,
    uint32_t now,
    uint8_t rng_pct) {
    if(deed >= DolphinDeedMAX) return false;

    s->exp += weight;
    DigiviceType type = digivice_rules_deed_type(deed);
    if(type == DigiviceTypeNone) return false;
    add_u16(&s->data[type], 1);

    bool eligible = digivice_rules_is_spawn_deed(deed) &&
                    digivice_rules_stage(s) >= DigiviceStageRookie &&
                    !digivice_rules_virux_active(s, now);
    if(!eligible || rng_pct >= DIGIVICE_SPAWN_PCT) return false;

    s->virux_type = type;
    s->virux_expires = now + DIGIVICE_VIRUX_TTL_S;
    return true;
}

bool digivice_rules_evolve(DigiviceState* s, uint32_t now) {
    // The pending cutscene must be acknowledged first or it would be overwritten unseen.
    if(s->evo_from != DigiviceFormNone) return false;
    switch(s->form) {
    case DigiviceFormByteegg:
        if(now >= s->born_at && now - s->born_at >= DIGIVICE_HATCH_S) {
            evolve_to(s, DigiviceFormBitling);
            return true;
        }
        return false;
    case DigiviceFormBitling:
        if(s->exp >= DIGIVICE_ROOKIE_EXP) {
            evolve_to(s, DigiviceFormBytemon);
            return true;
        }
        return false;
    case DigiviceFormBytemon:
        if(s->exp >= DIGIVICE_CHAMPION_EXP) {
            // Strict > keeps the first type on ties: NFC, SUBGHZ, IR, USB.
            uint8_t best = DigiviceTypeNfc;
            for(uint8_t t = 1; t < DigiviceTypeCount; t++) {
                if(s->data[t] > s->data[best]) best = t;
            }
            evolve_to(s, champion_of[best]);
            return true;
        }
        return false;
    default:
        return false;
    }
}

void digivice_rules_ack_evolution(DigiviceState* s) {
    s->evo_from = DigiviceFormNone;
}

static const uint16_t base_damage[DigiviceStageCount] = DIGIVICE_BASE_DAMAGE;
static const uint16_t player_hp[DigiviceStageCount] = DIGIVICE_PLAYER_HP;
static const uint16_t virux_hp[DigiviceStageCount] = DIGIVICE_VIRUX_HP;
static const uint16_t virux_atk[DigiviceStageCount] = DIGIVICE_VIRUX_ATK;

// The type each attacker type is strong against: NFC > USB > IR > SUBGHZ > NFC.
static const uint8_t beats[DigiviceTypeCount] = {
    [DigiviceTypeNfc] = DigiviceTypeUsb,
    [DigiviceTypeUsb] = DigiviceTypeIr,
    [DigiviceTypeIr] = DigiviceTypeSubGhz,
    [DigiviceTypeSubGhz] = DigiviceTypeNfc,
};

void digivice_rules_battle_result(DigiviceState* s, DigiviceBattleResult result, uint32_t now) {
    if(result == DigiviceBattleResultWin) {
        s->exp += 2 * DIGIVICE_WIN_BASE_EXP;
        if(s->virux_type < DigiviceTypeCount) add_u16(&s->data[s->virux_type], DIGIVICE_WIN_DATA);
        add_u16(&s->wins, 1);
    } else if(result == DigiviceBattleResultLose) {
        add_u16(&s->losses, 1);
        set_corruption(s, (int32_t)s->corruption + DIGIVICE_CORRUPTION_LOSE_GAIN, now);
    }
    s->virux_type = DigiviceTypeNone;
}

uint16_t digivice_rules_type_x10(DigiviceType attacker, DigiviceType defender) {
    if(attacker >= DigiviceTypeCount || defender >= DigiviceTypeCount) return 10;
    return beats[attacker] == defender ? DIGIVICE_TYPE_ADV_X10 : 10;
}

uint16_t digivice_rules_damage(
    uint16_t base,
    uint16_t strength,
    bool in_zone,
    DigiviceType attacker,
    DigiviceType defender) {
    uint32_t timing = in_zone ? DIGIVICE_TIMING_HIT_X10 : DIGIVICE_TIMING_MISS_X10;
    uint32_t damage =
        ((uint32_t)base + strength) * timing * digivice_rules_type_x10(attacker, defender) / 100;
    if(damage == 0) damage = 1;
    return damage > UINT16_MAX ? UINT16_MAX : (uint16_t)damage;
}

void digivice_battle_init(DigiviceBattle* b, const DigiviceState* s) {
    DigiviceStage stage = digivice_rules_stage(s);
    b->stage = stage;
    b->player_type = digivice_rules_player_type(s);
    b->foe_type = s->virux_type;
    b->player_hp_max = b->player_hp = player_hp[stage];
    b->foe_hp_max = b->foe_hp = virux_hp[stage];
    b->defending = false;
}

static void hurt(uint16_t* hp, uint16_t damage) {
    *hp = damage >= *hp ? 0 : *hp - damage;
}

uint16_t digivice_battle_player_attack(
    DigiviceBattle* b,
    const DigiviceState* s,
    bool in_zone,
    uint8_t rng_pct) {
    if(digivice_rules_attack_misses(s, rng_pct)) return 0;
    uint16_t damage = digivice_rules_damage(
        base_damage[b->stage],
        s->strength,
        in_zone,
        (DigiviceType)b->player_type,
        (DigiviceType)b->foe_type);
    hurt(&b->foe_hp, damage);
    return damage;
}

void digivice_battle_defend(DigiviceBattle* b) {
    b->defending = true;
}

bool digivice_battle_flee(uint8_t rng_pct) {
    return rng_pct < DIGIVICE_FLEE_PCT;
}

uint16_t digivice_battle_foe_attack(DigiviceBattle* b) {
    uint32_t damage =
        (uint32_t)virux_atk[b->stage] *
        digivice_rules_type_x10((DigiviceType)b->foe_type, (DigiviceType)b->player_type) / 10;
    if(b->defending) damage /= 2;
    if(damage == 0) damage = 1;
    b->defending = false;
    hurt(&b->player_hp, (uint16_t)damage);
    return (uint16_t)damage;
}

DigiviceBattleOutcome digivice_battle_outcome(const DigiviceBattle* b) {
    if(b->foe_hp == 0) return DigiviceBattleWon;
    if(b->player_hp == 0) return DigiviceBattleLost;
    return DigiviceBattleOngoing;
}
