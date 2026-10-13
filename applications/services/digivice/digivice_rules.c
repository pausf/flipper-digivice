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
