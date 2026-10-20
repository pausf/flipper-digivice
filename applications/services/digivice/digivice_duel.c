#include "digivice_duel.h"
#include "digivice_config.h"
#include "digivice_city.h"

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

#define FOE_POWER(p) ((uint16_t)((p)*DIGIVICE_FOE_POWER_PCT / 100))

static const uint8_t stage_shot[DigiviceStageCount] = DIGIVICE_STAGE_SHOT;
static const uint16_t virux_power[DigiviceStageCount] = DIGIVICE_VIRUX_POWER;
static const uint16_t stage_power[DigiviceStageCount] = DIGIVICE_STAGE_POWER;

void digivice_duel_init(
    DigiviceDuel* d,
    const DigiviceState* s,
    DigiviceEncounterKind kind,
    uint32_t now) {
    memset(d, 0, sizeof(DigiviceDuel));
    DigiviceStage stage = digivice_rules_stage(s);
    uint8_t tier = digivice_duel_tier(stage);
    uint8_t player_shot = stage_shot[stage];
    d->kind = kind;
    d->item_shield = s->item == DigiviceItemShield;
    d->glitchy = digivice_rules_glitchy(s);

    DigiviceFighterSpec pet = {
        .power = digivice_rules_player_power(s, now),
        .attr = digivice_rules_form_attr(s),
        .tier = tier,
        .seed = s->form,
        .force_util = DIGIVICE_MOVE_NONE,
        .spd_bonus = (uint8_t)(DIGIVICE_VIGOR_SPD * s->vigor),
        .shot = player_shot,
    };
    digivice_fighter_init(&d->f[DigiviceSidePlayer], &pet);

    DigiviceFighterSpec foe = {.force_util = DIGIVICE_MOVE_NONE, .shot = player_shot};
    if(kind == DigiviceEncounterBoss) {
        d->foe = s->area;
        foe.power = FOE_POWER(digivice_expedition_boss_power(s->area));
        foe.attr = digivice_expedition_boss_attr(s->area);
        foe.tier = (uint8_t)(2 + s->area / 3);
        foe.seed = s->area;
        foe.force_util = s->area & 1 ? DigiviceMoveSobrecarga : DigiviceMoveEscudoDatos;
        foe.shot = s->area == DIGIVICE_AREA_COUNT - 1 ? DIGIVICE_SHOT_BEAM : DIGIVICE_SHOT_BOSS;
    } else if(kind == DigiviceEncounterVirux) {
        d->foe = s->virux_type;
        foe.power = virux_power[stage];
        foe.attr = digivice_rules_type_attr((DigiviceType)s->virux_type);
        foe.tier = tier;
        foe.seed = s->virux_type;
        foe.force_util = DigiviceMoveSobrecarga;
    } else if(digivice_homage_code(s->enc_foe)) {
        d->foe_class = DigiviceFoeClassHomage;
        d->foe = (uint8_t)(s->enc_foe - DIGIVICE_ENC_HOMAGE);
        foe.power = (uint16_t)(stage_power[stage] * DIGIVICE_HOMAGE_X10 / 10);
        foe.attr = digivice_homage_attr(d->foe);
        foe.tier = tier;
        foe.seed = d->foe;
    } else if(digivice_city_code(s->enc_foe)) {
        d->foe_class = DigiviceFoeClassCity;
        d->foe = (uint8_t)(s->enc_foe - DIGIVICE_ENC_CITY);
        foe.power = FOE_POWER(digivice_city_monster_power(s->area));
        foe.attr = digivice_city_monster_attr(d->foe);
        foe.tier = 2;
        foe.seed = d->foe;
        foe.shot = DIGIVICE_SHOT_CHAMP_FOE;
    } else {
        DigiviceFoe pool = digivice_expedition_pool_foe(s->area, s->enc_foe <= 1 ? s->enc_foe : 0);
        d->foe = pool;
        foe.power = FOE_POWER(digivice_expedition_foe_power(s->area, pool));
        foe.attr = digivice_expedition_foe_attr(pool);
        bool champion = digivice_expedition_foe_champion(pool);
        foe.tier = champion ? 2 : 1;
        foe.seed = pool;
        foe.shot = champion ? DIGIVICE_SHOT_CHAMP_FOE : DIGIVICE_SHOT_ROOKIE_FOE;
    }
    digivice_fighter_init(&d->f[DigiviceSideFoe], &foe);
}

bool digivice_duel_shield_ready(const DigiviceDuel* d) {
    return d->item_shield && !d->shield_armed;
}

bool digivice_duel_arm_shield(DigiviceDuel* d) {
    if(!digivice_duel_shield_ready(d)) return false;
    d->shield_armed = true;
    return true;
}

static int8_t priority(const DigiviceFighter* f, uint8_t slot) {
    return slot < DIGIVICE_SLOTS && digivice_move(f->moves[slot])->fx == DigiviceFxQuick ? 1 : 0;
}

static uint16_t speed(const DigiviceFighter* f) {
    return f->glitch ? f->spd / 2 : f->spd;
}

bool digivice_duel_player_first(const DigiviceDuel* d, uint8_t player_slot, uint8_t foe_slot) {
    int8_t pp = priority(&d->f[DigiviceSidePlayer], player_slot);
    int8_t pf = priority(&d->f[DigiviceSideFoe], foe_slot);
    if(pp != pf) return pp > pf;
    return speed(&d->f[DigiviceSidePlayer]) >= speed(&d->f[DigiviceSideFoe]);
}

static bool apply_fx(DigiviceFighter* target, uint8_t fx) {
    switch(fx) {
    case DigiviceFxAtkDown:
        if(target->atk_low >= DIGIVICE_STAGE_MAX) return false;
        target->atk_low++;
        return true;
    case DigiviceFxDefDown:
        if(target->def_low >= DIGIVICE_STAGE_MAX) return false;
        target->def_low++;
        return true;
    case DigiviceFxGlitch:
        if(target->glitch) return false;
        target->glitch = true;
        return true;
    default:
        return false;
    }
}

DigiviceStep digivice_duel_act(
    DigiviceDuel* d,
    uint8_t actor,
    uint8_t slot,
    bool crit_bonus,
    const DigiviceRolls* rolls) {
    DigiviceFighter* me = &d->f[actor];
    DigiviceFighter* you = &d->f[1 - actor];
    DigiviceStep step = {.actor = actor, .slot = slot, .result = DigiviceStepDone};

    if(me->glitch && rolls->para < DIGIVICE_GLITCH_SKIP_PCT) {
        step.result = DigiviceStepSkipped;
        return step;
    }
    if(slot >= DIGIVICE_SLOTS || me->pp[slot] == 0) slot = DIGIVICE_SLOT_STRUGGLE;
    step.slot = slot;
    step.move = slot < DIGIVICE_SLOTS ? me->moves[slot] : DigiviceMoveLucha;
    if(slot < DIGIVICE_SLOTS) me->pp[slot]--;
    const DigiviceMove* m = digivice_move(step.move);

    bool corrupt_fail = actor == DigiviceSidePlayer && d->glitchy &&
                        rolls->miss < DIGIVICE_CORRUPTION_MISS_PCT;
    if(corrupt_fail || (m->acc && rolls->acc >= m->acc)) {
        step.result = DigiviceStepMissed;
        return step;
    }

    if(m->power) {
        if(actor == DigiviceSideFoe && d->shield_armed) {
            d->shield_armed = false;
            d->shield_used = true;
            step.result = DigiviceStepShielded;
            return step;
        }
        step.crit = crit_bonus || rolls->crit % DIGIVICE_CRIT_ODDS == 0;
        step.edge = digivice_rules_attr_adv(m->attr, you->attr);
        uint32_t dmg = digivice_duel_damage(me, you, m, step.crit, rolls->var);
        if(me->charged) {
            dmg *= DIGIVICE_CHARGE_X;
            me->charged = false;
        }
        if(you->guard) {
            dmg = (dmg + 1) / 2;
            you->guard = false;
        }
        if(dmg > you->hp) dmg = you->hp;
        you->hp = (uint16_t)(you->hp - dmg);
        step.damage = (uint16_t)dmg;
        if(rolls->fx < m->arg && apply_fx(you, m->fx)) step.fx = m->fx;
        return step;
    }

    switch(m->fx) {
    case DigiviceFxHeal: {
        uint32_t amount = (uint32_t)me->hp_max * m->arg / 100;
        if(amount > (uint32_t)(me->hp_max - me->hp)) amount = (uint32_t)(me->hp_max - me->hp);
        me->hp = (uint16_t)(me->hp + amount);
        step.healed = (uint16_t)amount;
        step.fx = DigiviceFxHeal;
        break;
    }
    case DigiviceFxShield:
        me->guard = true;
        step.fx = DigiviceFxShield;
        break;
    case DigiviceFxCharge:
        me->charged = true;
        step.fx = DigiviceFxCharge;
        break;
    default:
        if(apply_fx(you, m->fx)) step.fx = m->fx;
        break;
    }
    return step;
}

void digivice_duel_end_round(DigiviceDuel* d) {
    if(d->round < UINT8_MAX) d->round++;
}

DigiviceBattleOutcome digivice_duel_outcome(const DigiviceDuel* d) {
    const DigiviceFighter* p = &d->f[DigiviceSidePlayer];
    const DigiviceFighter* q = &d->f[DigiviceSideFoe];
    if(q->hp == 0) return DigiviceBattleWon;
    if(p->hp == 0) return DigiviceBattleLost;
    if(d->round < DIGIVICE_TURN_CAP) return DigiviceBattleOngoing;
    uint32_t mine = (uint32_t)p->hp * q->hp_max;
    uint32_t theirs = (uint32_t)q->hp * p->hp_max;
    if(mine > theirs) return DigiviceBattleWon;
    if(theirs > mine) return DigiviceBattleLost;
    return DigiviceBattleDrawn;
}
