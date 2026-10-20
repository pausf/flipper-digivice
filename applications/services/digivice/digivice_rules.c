#include "digivice_rules.h"
#include "digivice_config.h"
#include "digivice_expedition.h"
#include "digivice_roster.h"
#include "digivice_evolution.h"

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

static const uint16_t area_lengths[DIGIVICE_AREA_COUNT] = DIGIVICE_AREA_LENGTHS;

uint16_t digivice_rules_area_length(uint8_t area) {
    return area_lengths[area < DIGIVICE_AREA_COUNT ? area : DIGIVICE_AREA_COUNT - 1];
}

static void v3_defaults(DigiviceState* s) {
    s->area = 0;
    s->cleared = 0;
    s->enc_foe = DIGIVICE_ENC_NONE;
    s->mid_state = 0;
    s->item = DigiviceItemNone;
    s->hike_count = 0;
    s->scan_count = 0;
    s->reserved3 = 0;
    s->distance = digivice_rules_area_length(0);
    s->paces_since_fight = 0;
    s->pace_acc_s = 0;
    s->enc_expires = 0;
    s->bonus_window_start = 0;
    s->injured_until = 0;
    s->rng = (s->last_tick ^ s->born_at ^ 0x9E3779B9UL) | 1UL;
}

static void v4_defaults(DigiviceState* s, uint32_t stage_since) {
    s->vigor = DIGIVICE_VIGOR_START;
    s->call_flags = 0;
    s->feed_flags = DIGIVICE_FEED_ARM_HUNGER | DIGIVICE_FEED_ARM_VIGOR;
    s->evo_mistakes = 0;
    s->train_count = 0;
    s->battles = 0;
    s->wins_stage = 0;
    s->overfeed = 0;
    s->night_owl = 0;
    memset(s->reserved4, 0, sizeof(s->reserved4));
    s->call_hunger_s = 0;
    s->call_vigor_s = 0;
    s->stage_since = stage_since;
    s->vigor_acc_s = 0;
}

void digivice_rules_migrate_v3(const DigiviceStateV3* v3, DigiviceState* s) {
    memset(s, 0, sizeof(*s));
    // The v4 struct starts with the exact v3 layout (static_assert in digivice_rules.h).
    memcpy(s, v3, sizeof(*v3));
    v4_defaults(s, v3->last_tick);
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
    s->lang = DigiviceLangEs;
    s->mode = DigiviceModeNormal;
    s->light_on = 1;
    v3_defaults(s);
    v4_defaults(s, now);
}

void digivice_rules_reset(DigiviceState* s, uint32_t now) {
    uint8_t lang = s->lang;
    uint8_t mode = s->mode;
    digivice_rules_init(s, now);
    s->lang = lang;
    s->mode = mode;
}

void digivice_rules_migrate_v1(const DigiviceStateV1* v1, DigiviceStateV2* s) {
    memset(s, 0, sizeof(DigiviceStateV2));
    s->form = v1->form;
    s->prev_form = v1->prev_form;
    s->evo_from = v1->evo_from;
    s->hunger = v1->hunger;
    s->mood = v1->mood;
    s->corruption = v1->corruption;
    s->virux_type = v1->virux_type;
    s->reserved = v1->reserved;
    s->strength = v1->strength;
    s->wins = v1->wins;
    s->losses = v1->losses;
    memcpy(s->data, v1->data, sizeof(s->data));
    s->exp = v1->exp;
    s->virux_expires = v1->virux_expires;
    s->last_tick = v1->last_tick;
    s->born_at = v1->born_at;
    s->clean_since = v1->clean_since;
    s->hunger_acc_s = v1->hunger_acc_s;
    s->mood_acc_s = v1->mood_acc_s;
    s->corrupt_acc_s = v1->corrupt_acc_s;
    s->lang = DigiviceLangEs;
    s->mode = DigiviceModeNormal;
    s->light_on = 1;
}

void digivice_rules_migrate_v2(const DigiviceStateV2* v2, DigiviceState* s) {
    memset(s, 0, sizeof(DigiviceState));
    // The v3 struct starts with the exact v2 layout (static_assert in digivice_rules.h).
    memcpy(s, v2, sizeof(DigiviceStateV2));
    v3_defaults(s);
    v4_defaults(s, s->last_tick);
}

void digivice_rules_sanitize(DigiviceState* s) {
    if(s->lang >= DigiviceLangCount) s->lang = DigiviceLangEs;
    if(s->vigor > DIGIVICE_VIGOR_MAX) s->vigor = DIGIVICE_VIGOR_MAX;
    s->call_flags &= DIGIVICE_CALL_HUNGER | DIGIVICE_CALL_VIGOR;
    s->feed_flags &= DIGIVICE_FEED_ARM_HUNGER | DIGIVICE_FEED_ARM_VIGOR;
    if(s->call_hunger_s > DIGIVICE_CALL_WINDOW_S) s->call_hunger_s = DIGIVICE_CALL_WINDOW_S;
    if(s->call_vigor_s > DIGIVICE_CALL_WINDOW_S) s->call_vigor_s = DIGIVICE_CALL_WINDOW_S;
    if(s->vigor_acc_s >= DIGIVICE_VIGOR_DECAY_S) s->vigor_acc_s = 0;
    if(s->mode >= DigiviceModeCount) s->mode = DigiviceModeNormal;
    if(s->area >= DIGIVICE_AREA_COUNT) s->area = 0;
    if(s->enc_foe > 1) s->enc_foe = DIGIVICE_ENC_NONE;
    if(s->mid_state > 2) s->mid_state = 2;
    if(s->item >= DigiviceItemCount) s->item = DigiviceItemNone;
    uint16_t length = digivice_rules_area_length(s->area);
    if(s->distance > length) s->distance = length;
    if(s->hike_count > DIGIVICE_HIKE_CAP) s->hike_count = DIGIVICE_HIKE_CAP;
    if(s->scan_count > DIGIVICE_SCAN_CAP) s->scan_count = DIGIVICE_SCAN_CAP;
    if(s->paces_since_fight > DIGIVICE_ENCOUNTER_GUARANTEE) {
        s->paces_since_fight = DIGIVICE_ENCOUNTER_GUARANTEE;
    }
    if(s->rng == 0) s->rng = 1;
    // Only one encounter can be pending: the VIRUX keeps its slot.
    if(s->virux_type != DigiviceTypeNone && s->enc_foe != DIGIVICE_ENC_NONE) {
        s->enc_foe = DIGIVICE_ENC_NONE;
        s->enc_expires = 0;
    }
}

uint32_t digivice_rules_rng(DigiviceState* s) {
    uint32_t x = s->rng ? s->rng : 1;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s->rng = x;
    return x;
}

uint8_t digivice_rules_rng_pct(DigiviceState* s) {
    return (uint8_t)(digivice_rules_rng(s) % 100);
}

void digivice_rules_mix_entropy(DigiviceState* s, uint32_t entropy) {
    s->rng ^= entropy;
    if(s->rng == 0) s->rng = 1;
}

// The attribute each attribute beats; Nulo beats nothing and loses to nothing.
static const uint8_t attr_beats[DigiviceAttrNulo] = {
    [DigiviceAttrPortador] = DigiviceAttrRuido,
    [DigiviceAttrRuido] = DigiviceAttrCifrado,
    [DigiviceAttrCifrado] = DigiviceAttrPortador,
};

int8_t digivice_rules_attr_adv(uint8_t attacker, uint8_t defender) {
    if(attacker >= DigiviceAttrNulo || defender >= DigiviceAttrNulo) return 0;
    if(attr_beats[attacker] == defender) return 1;
    if(attr_beats[defender] == attacker) return -1;
    return 0;
}

DigiviceAttr digivice_rules_form_attr(const DigiviceState* s) {
    const DigiviceRosterEntry* e = digivice_roster(s->form);
    return e ? (DigiviceAttr)e->attr : DigiviceAttrNulo;
}

DigiviceAttr digivice_rules_type_attr(DigiviceType type) {
    switch(type) {
    case DigiviceTypeNfc:
        return DigiviceAttrCifrado;
    case DigiviceTypeSubGhz:
        return DigiviceAttrPortador;
    case DigiviceTypeIr:
        return DigiviceAttrRuido;
    default:
        return DigiviceAttrNulo;
    }
}

bool digivice_rules_set_lang(DigiviceState* s, uint8_t lang) {
    if(lang >= DigiviceLangCount) return false;
    s->lang = lang;
    return true;
}

bool digivice_rules_set_mode(DigiviceState* s, uint8_t mode) {
    if(mode >= DigiviceModeCount) return false;
    s->mode = mode;
    return true;
}

DigiviceStage digivice_rules_stage(const DigiviceState* s) {
    uint8_t form = s->form == DigiviceFormVirubyte ? s->prev_form : s->form;
    const DigiviceRosterEntry* e = digivice_roster(form);
    return e ? (DigiviceStage)e->stage : DigiviceStageEgg;
}

bool digivice_rules_is_sleep_hour(uint8_t hour) {
    return hour >= DIGIVICE_SLEEP_START_HOUR || hour < DIGIVICE_SLEEP_END_HOUR;
}

bool digivice_rules_is_asleep(uint32_t now) {
    return digivice_rules_is_sleep_hour((uint8_t)((now % 86400UL) / 3600UL));
}

bool digivice_rules_bedtime_started(const DigiviceState* s, uint32_t prev, uint32_t now) {
    // A long gap is a catch-up after the Flipper was off, not a bedtime the user was around for.
    if(now <= prev || now - prev > 5UL * DIGIVICE_TICK_S) return false;
    return digivice_rules_is_asleep(now) && !digivice_rules_is_asleep(prev) && s->light_on &&
           digivice_rules_stage(s) != DigiviceStageEgg;
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

static uint32_t until(uint32_t t, uint32_t deadline) {
    return deadline > t ? deadline - t : 0;
}

static void bump(uint8_t* counter) {
    if(*counter < UINT8_MAX) (*counter)++;
}

void digivice_rules_count_battle(DigiviceState* s, bool won) {
    bump(&s->battles);
    if(won) bump(&s->wins_stage);
}

bool digivice_rules_call_active(const DigiviceState* s) {
    return s->call_flags != 0;
}

// Poop and light still push the pet towards sickness (v2.1) but are no evolution mistakes.
static void add_mistake(DigiviceState* s) {
    bump(&s->care_mistakes);
}

// An ignored call is the only care mistake that counts for the evolution.
static void add_call_mistake(DigiviceState* s) {
    bump(&s->care_mistakes);
    bump(&s->evo_mistakes);
}

static void set_hunger(DigiviceState* s, uint8_t hunger, uint32_t now) {
    if(hunger == 0 && s->hunger > 0) s->starving_since = now;
    if(hunger > 0) s->starving_since = 0;
    if(hunger < DIGIVICE_HUNGER_MAX) s->feed_flags |= DIGIVICE_FEED_ARM_HUNGER;
    if(hunger == 0 && s->hunger > 0) s->call_flags |= DIGIVICE_CALL_HUNGER;
    s->hunger = hunger;
}

static void set_vigor(DigiviceState* s, uint8_t vigor) {
    if(vigor < DIGIVICE_VIGOR_MAX) s->feed_flags |= DIGIVICE_FEED_ARM_VIGOR;
    if(vigor == 0 && s->vigor > 0) s->call_flags |= DIGIVICE_CALL_VIGOR;
    s->vigor = vigor;
}

static bool corrupting(const DigiviceState* s) {
    return s->hunger == 0 || s->mood == 0 || s->sick;
}

static bool pooping(const DigiviceState* s) {
    return s->poop < DIGIVICE_POOP_MAX && digivice_rules_stage(s) >= DigiviceStageBaby;
}

static uint32_t mood_rate(const DigiviceState* s) {
    return (s->sick || s->poop >= DIGIVICE_POOP_MOOD_FAST) ? DIGIVICE_MOOD_FAST_X : 1;
}

// Wall-clock rules: they fire at the exact second, awake or asleep.
static void apply_due(DigiviceState* s, uint32_t t) {
    if(s->poop > 0 && t >= s->poop_dirty_since &&
       t - s->poop_dirty_since >= DIGIVICE_POOP_MISTAKE_S) {
        add_mistake(s);
        s->poop_dirty_since = t;
    }
    bool starved = s->starving_since != 0 && t >= s->starving_since &&
                   t - s->starving_since >= DIGIVICE_STARVE_SICK_S;
    if(!s->sick && (s->care_mistakes >= DIGIVICE_SICK_MISTAKES || starved)) {
        s->sick = 1;
        s->doses_needed = 1;
        s->sick_since = t;
    }
}

static uint32_t next_due(const DigiviceState* s, uint32_t t) {
    uint32_t step = UINT32_MAX;
    if(s->poop > 0) step = until(t, s->poop_dirty_since + DIGIVICE_POOP_MISTAKE_S);
    if(!s->sick && s->starving_since != 0) {
        step = min_u32(step, until(t, s->starving_since + DIGIVICE_STARVE_SICK_S));
    }
    return step;
}

static uint32_t awake_step(const DigiviceState* s, uint32_t step) {
    uint32_t rate = mood_rate(s);
    if(s->hunger > 0) step = min_u32(step, DIGIVICE_HUNGER_DECAY_S - s->hunger_acc_s);
    if(s->vigor > 0) step = min_u32(step, DIGIVICE_VIGOR_DECAY_S - s->vigor_acc_s);
    // Rounded up, so a doubled decay lands on or just past its boundary, never short of it.
    if(s->mood > 0) {
        step = min_u32(step, (DIGIVICE_MOOD_DECAY_S - s->mood_acc_s + rate - 1) / rate);
    }
    if(s->call_flags & DIGIVICE_CALL_HUNGER) {
        step = min_u32(step, DIGIVICE_CALL_WINDOW_S - s->call_hunger_s);
    }
    if(s->call_flags & DIGIVICE_CALL_VIGOR) {
        step = min_u32(step, DIGIVICE_CALL_WINDOW_S - s->call_vigor_s);
    }
    if(corrupting(s)) step = min_u32(step, 3600UL - s->corrupt_acc_s);
    if(pooping(s)) step = min_u32(step, DIGIVICE_POOP_EVERY_S - s->poop_acc_s);
    return step;
}

static void awake_advance(DigiviceState* s, uint32_t step, uint32_t t_end) {
    // Paces first: they use the state the step started with, like the care accumulators.
    digivice_expedition_walk(s, step, t_end);
    bool poops = pooping(s);
    if(s->call_flags & DIGIVICE_CALL_HUNGER) s->call_hunger_s += (uint16_t)step;
    if(s->call_flags & DIGIVICE_CALL_VIGOR) s->call_vigor_s += (uint16_t)step;
    if(s->call_hunger_s >= DIGIVICE_CALL_WINDOW_S) {
        s->call_flags &= (uint8_t)~DIGIVICE_CALL_HUNGER;
        s->call_hunger_s = 0;
        add_call_mistake(s);
    }
    if(s->call_vigor_s >= DIGIVICE_CALL_WINDOW_S) {
        s->call_flags &= (uint8_t)~DIGIVICE_CALL_VIGOR;
        s->call_vigor_s = 0;
        add_call_mistake(s);
    }
    if(s->hunger > 0) s->hunger_acc_s += step;
    if(s->vigor > 0) s->vigor_acc_s += step;
    // Doubled decay can overshoot by under `rate` seconds; the remainder is carried, not lost.
    if(s->mood > 0) s->mood_acc_s += step * mood_rate(s);
    if(corrupting(s)) s->corrupt_acc_s += step;
    if(poops) s->poop_acc_s += step;

    if(s->hunger_acc_s >= DIGIVICE_HUNGER_DECAY_S) {
        s->hunger_acc_s = 0;
        set_hunger(s, s->hunger - 1, t_end);
    }
    if(s->vigor_acc_s >= DIGIVICE_VIGOR_DECAY_S) {
        s->vigor_acc_s = 0;
        set_vigor(s, s->vigor - 1);
    }
    if(s->mood_acc_s >= DIGIVICE_MOOD_DECAY_S) {
        s->mood_acc_s -= DIGIVICE_MOOD_DECAY_S;
        s->mood--;
    }
    if(s->corrupt_acc_s >= 3600UL) {
        s->corrupt_acc_s -= 3600UL;
        set_corruption(s, (int32_t)s->corruption + DIGIVICE_CORRUPTION_PER_HOUR, t_end);
    }
    if(s->poop_acc_s >= DIGIVICE_POOP_EVERY_S) {
        s->poop_acc_s = 0;
        if(s->poop == 0) s->poop_dirty_since = t_end;
        s->poop++;
    }
}

static void asleep_advance(DigiviceState* s, uint32_t step) {
    // An egg has no care rules: it never poops and the light cannot bother it.
    if(!s->light_on || digivice_rules_stage(s) == DigiviceStageEgg) return;
    uint32_t before = s->light_on_asleep_s;
    s->light_on_asleep_s += step;
    // Crossing the threshold, not being past it, makes this once per sleep window.
    if(before < DIGIVICE_LIGHT_MISTAKE_S && s->light_on_asleep_s >= DIGIVICE_LIGHT_MISTAKE_S) {
        add_mistake(s);
        bump(&s->night_owl);
        if(s->mood > 0) s->mood--;
    }
}

// One clock hour (or less) that is entirely awake or entirely asleep.
static void run_segment(DigiviceState* s, uint32_t t, uint32_t to, bool asleep) {
    if(!asleep) {
        // The light comes back on when the sleep window ends.
        s->light_on = 1;
        s->light_on_asleep_s = 0;
    }
    while(t < to) {
        apply_due(s, t);
        uint32_t step = min_u32(to - t, next_due(s, t));
        if(asleep) {
            if(s->light_on && s->light_on_asleep_s < DIGIVICE_LIGHT_MISTAKE_S) {
                step = min_u32(step, DIGIVICE_LIGHT_MISTAKE_S - s->light_on_asleep_s);
            }
        } else {
            step = awake_step(s, step);
        }
        if(step == 0) step = 1; // a clock jump can leave a deadline behind t; never stall
        if(asleep) {
            asleep_advance(s, step);
        } else {
            awake_advance(s, step, t + step);
        }
        t += step;
    }
}

void digivice_rules_tick(DigiviceState* s, uint32_t now) {
    uint32_t prev = s->last_tick;
    uint32_t elapsed = now > prev ? now - prev : 0;
    elapsed = min_u32(elapsed, DIGIVICE_ELAPSED_MAX_S);
    uint32_t t = now - elapsed;

    // A migrated or hand-edited state can be at 0 hunger without a start time.
    if(s->hunger == 0 && s->starving_since == 0) s->starving_since = t;

    // Advance hour by hour, then event by event inside each hour, so 60 s ticks and one long
    // catch-up tick end in the same state.
    while(t < now) {
        uint32_t next = min_u32((t / 3600UL + 1UL) * 3600UL, now);
        run_segment(s, t, next, digivice_rules_is_asleep(t));
        t = next;
    }
    apply_due(s, now);

    // The second dose is due once, when this tick's window crosses sick_since + 12 h.
    uint32_t two_doses_at = s->sick_since + DIGIVICE_SICK_TWO_DOSES_S;
    if(s->sick && s->doses_needed == 1 && two_doses_at > prev && two_doses_at <= now) {
        s->doses_needed = 2;
    }

    if(s->virux_type != DigiviceTypeNone && now >= s->virux_expires) {
        s->virux_type = DigiviceTypeNone;
    }
    digivice_expedition_expire(s, now);
    if(s->injured_until != 0 && now >= s->injured_until) s->injured_until = 0;

    if(s->form == DigiviceFormVirubyte && s->corruption == 0 && s->clean_since != 0 &&
       now >= s->clean_since && now - s->clean_since >= DIGIVICE_CORRUPTION_RECOVERY_S) {
        evolve_to(s, s->prev_form);
    }

    s->last_tick = now;
}

bool digivice_rules_feed(DigiviceState* s, DigiviceFood food, uint32_t now) {
    bool meat = food == DigiviceFoodMeat;
    uint8_t arm = meat ? DIGIVICE_FEED_ARM_HUNGER : DIGIVICE_FEED_ARM_VIGOR;
    uint8_t level = meat ? s->hunger : s->vigor;
    uint8_t max = meat ? DIGIVICE_HUNGER_MAX : DIGIVICE_VIGOR_MAX;
    if(level >= max) {
        // A heart must drain before the next refusal counts.
        if((s->feed_flags & arm) && s->overfeed < UINT8_MAX) s->overfeed++;
        s->feed_flags &= (uint8_t)~arm;
        return false;
    }
    if(meat) {
        set_hunger(s, s->hunger + 1, now);
        s->call_flags &= (uint8_t)~DIGIVICE_CALL_HUNGER;
        s->call_hunger_s = 0;
    } else {
        set_vigor(s, s->vigor + 1);
        s->call_flags &= (uint8_t)~DIGIVICE_CALL_VIGOR;
        s->call_vigor_s = 0;
        add_u16(&s->strength, DIGIVICE_PROTEIN_STRENGTH);
    }
    set_corruption(s, (int32_t)s->corruption - DIGIVICE_CORRUPTION_FEED_DROP, now);
    return true;
}

DigiviceResult digivice_rules_can_train(const DigiviceState* s, uint32_t now) {
    if(s->sick) return DigiviceResultSick;
    if(digivice_rules_is_asleep(now)) return DigiviceResultAsleep;
    return DigiviceResultOk;
}

DigiviceResult digivice_rules_can_battle(const DigiviceState* s, uint32_t now) {
    if(s->sick) return DigiviceResultSick;
    if(digivice_rules_is_asleep(now)) return DigiviceResultAsleep;
    if(digivice_rules_stage(s) < DigiviceStageBaby) return DigiviceResultTooYoung;
    return DigiviceResultOk;
}

DigiviceResult digivice_rules_train(DigiviceState* s, bool success, uint32_t now) {
    DigiviceResult result = digivice_rules_can_train(s, now);
    if(result != DigiviceResultOk) return result;
    if(s->hunger > 0) set_hunger(s, s->hunger - 1, now);
    s->exp += success ? DIGIVICE_TRAIN_EXP_OK : DIGIVICE_TRAIN_EXP_FAIL;
    if(!success) return DigiviceResultOk;
    add_u16(&s->strength, 1);
    bump(&s->train_count);
    if(s->mood < DIGIVICE_MOOD_MAX) s->mood++;
    set_corruption(s, (int32_t)s->corruption - DIGIVICE_CORRUPTION_TRAIN_DROP, now);
    return DigiviceResultOk;
}

bool digivice_rules_clean(DigiviceState* s, uint32_t now) {
    (void)now;
    if(s->poop == 0) return false;
    s->poop = 0;
    s->poop_dirty_since = 0;
    if(s->mood < DIGIVICE_MOOD_MAX) s->mood++;
    return true;
}

bool digivice_rules_cure(DigiviceState* s, uint32_t now) {
    if(!s->sick) return false;
    if(s->doses_needed > 0) s->doses_needed--;
    if(s->doses_needed == 0) {
        s->sick = 0;
        s->care_mistakes = 0;
        s->sick_since = 0;
        // Still starving: the 24 h clock restarts, or the pet would relapse on the next tick.
        if(s->starving_since != 0) s->starving_since = now;
    }
    return true;
}

DigiviceResult digivice_rules_toggle_light(DigiviceState* s, uint32_t now) {
    if(!digivice_rules_is_asleep(now)) return DigiviceResultAwake;
    s->light_on = !s->light_on;
    return DigiviceResultOk;
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
    // One copy, not two: the service runs this on a 2 KB stack.
    DigiviceState y = *b;
    y.last_tick = a->last_tick;
    y.hunger_acc_s = a->hunger_acc_s;
    y.mood_acc_s = a->mood_acc_s;
    y.corrupt_acc_s = a->corrupt_acc_s;
    y.poop_acc_s = a->poop_acc_s;
    y.light_on_asleep_s = a->light_on_asleep_s;
    y.pace_acc_s = a->pace_acc_s;
    y.paces_since_fight = a->paces_since_fight;
    y.rng = a->rng;
    y.hike_count = a->hike_count;
    y.scan_count = a->scan_count;
    y.bonus_window_start = a->bonus_window_start;
    y.vigor_acc_s = a->vigor_acc_s;
    y.call_hunger_s = a->call_hunger_s;
    y.call_vigor_s = a->call_vigor_s;
    // Paces are bookkeeping; only reaching the boss (distance 0) is visible.
    if((a->distance == 0) == (b->distance == 0)) y.distance = a->distance;
    return memcmp(a, &y, sizeof(DigiviceState)) == 0;
}

DigiviceType digivice_rules_player_type(const DigiviceState* s) {
    if(digivice_rules_stage(s) < DigiviceStageChampion) return DigiviceTypeNone;
    switch(digivice_rules_form_attr(s)) {
    case DigiviceAttrCifrado:
        return DigiviceTypeNfc;
    case DigiviceAttrPortador:
        return DigiviceTypeSubGhz;
    case DigiviceAttrRuido:
        return DigiviceTypeIr;
    default:
        return DigiviceTypeUsb;
    }
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

void digivice_rules_add_corruption(DigiviceState* s, int32_t delta, uint32_t now) {
    set_corruption(s, (int32_t)s->corruption + delta, now);
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
    if(deed >= DolphinDeedMAX || s->mode != DigiviceModeHacker) return false;

    s->exp += weight;
    DigiviceType type = digivice_rules_deed_type(deed);
    if(type == DigiviceTypeNone) return false;
    add_u16(&s->data[type], 1);

    bool spawn_deed = digivice_rules_is_spawn_deed(deed);
    // A VIRUX never joins a pending trail encounter: one encounter at a time.
    bool eligible = spawn_deed && digivice_rules_stage(s) >= DigiviceStageRookie &&
                    !digivice_rules_virux_active(s, now) &&
                    !digivice_expedition_trail_pending(s, now);
    bool spawned = eligible && rng_pct < DIGIVICE_SPAWN_PCT;
    if(spawned) {
        s->virux_type = type;
        s->virux_expires = now + DIGIVICE_VIRUX_TTL_S;
    }
    if(spawn_deed) digivice_expedition_scan(s, now);
    return spawned;
}

// A real evolution: new clock, counters back to 0. The corruption transform uses evolve_to.
static void reset_stage(DigiviceState* s, uint32_t now) {
    s->stage_since = now;
    s->train_count = 0;
    s->battles = 0;
    s->wins_stage = 0;
    s->overfeed = 0;
    s->night_owl = 0;
    s->evo_mistakes = 0;
}

bool digivice_rules_evolve(DigiviceState* s, uint32_t now) {
    // The pending cutscene must be acknowledged first or it would be overwritten unseen.
    if(s->evo_from != DigiviceFormNone) return false;
    uint8_t next;
    if(s->form == DigiviceFormByteegg) {
        if(now < s->stage_since || now - s->stage_since < DIGIVICE_HATCH_S) return false;
        next = digivice_rules_rng_pct(s) < 50 ? DigiviceFormPipo : DigiviceFormChispi;
    } else {
        next = digivice_evolution_next(s, now);
    }
    if(next == DigiviceFormNone) return false;
    evolve_to(s, next);
    reset_stage(s, now);
    return true;
}

static uint8_t upgrade_form(uint8_t form) {
    switch(form) {
    case DigiviceFormBitling:
        return DigiviceFormPipo;
    case DigiviceFormBytemon:
        return DigiviceFormKorno;
    case DigiviceFormTagdrake:
        return DigiviceFormCerrojo;
    case DigiviceFormWavedrake:
        return DigiviceFormAriete;
    case DigiviceFormBeamdrake:
        return DigiviceFormFuzzcan;
    case DigiviceFormPlugdrake:
        return DigiviceFormChatarro;
    default:
        return form;
    }
}

bool digivice_rules_upgrade_legacy(DigiviceState* s, uint32_t now) {
    uint8_t form = upgrade_form(s->form);
    uint8_t prev = upgrade_form(s->prev_form);
    uint8_t from = upgrade_form(s->evo_from);
    bool changed = form != s->form || prev != s->prev_form || from != s->evo_from;
    s->form = form;
    s->prev_form = prev;
    s->evo_from = from;
    if(changed) reset_stage(s, now);
    return changed;
}

void digivice_rules_ack_evolution(DigiviceState* s) {
    s->evo_from = DigiviceFormNone;
}
