#include "digivice_duel.h"
#include "digivice_config.h"

#include <string.h>

static const uint8_t stage_x100[DIGIVICE_STAGE_MAX + 1] = DIGIVICE_STAGE_X100;

uint8_t digivice_duel_tier(DigiviceStage stage) {
    return stage <= DigiviceStageBaby2 ? 0 : (uint8_t)(stage - DigiviceStageBaby2);
}

void digivice_fighter_init(DigiviceFighter* f, const DigiviceFighterSpec* spec) {
    memset(f, 0, sizeof(DigiviceFighter));
    f->level = (uint8_t)(DIGIVICE_LEVEL_BASE + spec->power / DIGIVICE_LEVEL_DIV);
    f->hp_max = (uint16_t)(DIGIVICE_HP_MULT * f->level + DIGIVICE_HP_BASE);
    f->hp = f->hp_max;
    f->atk = (uint16_t)(DIGIVICE_STAT_BASE + spec->power);
    f->def = f->atk;
    f->spd = (uint16_t)(DIGIVICE_SPD_BASE + spec->power / 2 + spec->spd_bonus);
    f->attr = spec->attr;
    f->shot = spec->shot;
    digivice_moves_assign(spec->tier, spec->attr, spec->seed, spec->force_util, f->moves);
    for(uint8_t i = 0; i < DIGIVICE_SLOTS; i++) f->pp[i] = digivice_move(f->moves[i])->pp;
}

uint16_t digivice_duel_damage(
    const DigiviceFighter* attacker,
    const DigiviceFighter* defender,
    const DigiviceMove* move,
    bool crit,
    uint8_t var) {
    uint32_t atk = (uint32_t)attacker->atk * stage_x100[attacker->atk_low] / 100;
    uint32_t def = (uint32_t)defender->def * stage_x100[defender->def_low] / 100;
    if(def == 0) def = 1;
    uint32_t dmg = (2U * attacker->level / 5 + 2) * move->power * atk / def / 50 + 2;
    int8_t edge = digivice_rules_attr_adv(move->attr, defender->attr);
    if(edge > 0) {
        dmg = dmg * DIGIVICE_ADV_X100 / 100;
    } else if(edge < 0) {
        dmg = dmg * DIGIVICE_DIS_X100 / 100;
    }
    if(move->attr == attacker->attr) dmg = dmg * DIGIVICE_STAB_X100 / 100;
    if(crit) dmg = dmg * DIGIVICE_CRIT_X100 / 100;
    dmg = dmg * (DIGIVICE_ROLL_MIN + var * (100 - DIGIVICE_ROLL_MIN + 1) / 100) / 100;
    return (uint16_t)(dmg ? dmg : 1);
}

static uint16_t pick_weight(
    const DigiviceFighter* me,
    const DigiviceFighter* foe,
    const DigiviceMove* m) {
    if(m->power) {
        uint16_t w = DIGIVICE_AI_ATTACK_BASE + m->power / 4;
        int8_t edge = digivice_rules_attr_adv(m->attr, foe->attr);
        if(edge > 0) {
            w *= 2;
        } else if(edge < 0) {
            w /= 2;
        }
        if(m->attr == me->attr) w += w / 5;
        return w ? w : 1;
    }
    switch(m->fx) {
    case DigiviceFxHeal: {
        bool low = (uint32_t)me->hp * 100 < (uint32_t)me->hp_max * DIGIVICE_HEAL_LOW_PCT;
        return low && me->hp < me->hp_max ? 120 : 0;
    }
    case DigiviceFxShield:
        return me->guard ? 0 : 12;
    case DigiviceFxCharge:
        return me->charged ? 0 : 14;
    case DigiviceFxAtkDown:
        return foe->atk_low < DIGIVICE_STAGE_MAX ? 12 : 0;
    case DigiviceFxDefDown:
        return foe->def_low < DIGIVICE_STAGE_MAX ? 12 : 0;
    default:
        return 0;
    }
}

bool digivice_fighter_spent(const DigiviceFighter* f) {
    for(uint8_t i = 0; i < DIGIVICE_SLOTS; i++) {
        if(f->pp[i]) return false;
    }
    return true;
}

uint8_t digivice_fighter_pick(
    const DigiviceFighter* me,
    const DigiviceFighter* foe,
    uint8_t roll) {
    uint16_t w[DIGIVICE_SLOTS];
    uint32_t sum = 0;
    for(uint8_t i = 0; i < DIGIVICE_SLOTS; i++) {
        w[i] = me->pp[i] ? pick_weight(me, foe, digivice_move(me->moves[i])) : 0;
        sum += w[i];
    }
    if(sum == 0) return DIGIVICE_SLOT_STRUGGLE;
    uint32_t target = (uint32_t)roll * sum / 256;
    for(uint8_t i = 0; i < DIGIVICE_SLOTS; i++) {
        if(target < w[i]) return i;
        target -= w[i];
    }
    return DIGIVICE_SLOT_STRUGGLE;
}
