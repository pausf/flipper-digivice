#include "digivice_battle.h"
#include "digivice_config.h"

#include <string.h>

static const uint16_t stage_power[DigiviceStageCount] = DIGIVICE_STAGE_POWER;
static const uint8_t stage_shot[DigiviceStageCount] = DIGIVICE_STAGE_SHOT;
static const uint16_t virux_power[DigiviceStageCount] = DIGIVICE_VIRUX_POWER;
static const uint8_t charge_x10[DigiviceChargeCount] = DIGIVICE_CHARGE_X10;

static void inc_u16(uint16_t* value) {
    if(*value < UINT16_MAX) (*value)++;
}

uint8_t digivice_rules_hit_pct(uint16_t p, uint16_t q, int8_t adv) {
    uint32_t sum = (uint32_t)p + q;
    int32_t pct = sum ? (int32_t)((200UL * p + sum) / (2UL * sum)) : 50;
    pct += DIGIVICE_ATTR_PCT * adv;
    if(pct < DIGIVICE_HIT_MIN_PCT) pct = DIGIVICE_HIT_MIN_PCT;
    if(pct > DIGIVICE_HIT_MAX_PCT) pct = DIGIVICE_HIT_MAX_PCT;
    return (uint8_t)pct;
}

bool digivice_rules_injured(const DigiviceState* s, uint32_t now) {
    return s->injured_until != 0 && now < s->injured_until;
}

uint16_t digivice_rules_player_power(const DigiviceState* s, uint32_t now) {
    uint16_t strength = s->strength < DIGIVICE_STRENGTH_CAP ? s->strength : DIGIVICE_STRENGTH_CAP;
    uint16_t power = stage_power[digivice_rules_stage(s)] + strength;
    if(digivice_rules_injured(s, now)) power = power * 3 / 4;
    return power;
}

uint16_t digivice_rules_charged_power(uint16_t power, DigiviceCharge tier) {
    if(tier >= DigiviceChargeCount) tier = DigiviceChargeNormal;
    return (uint16_t)((uint32_t)power * charge_x10[tier] / 10);
}

uint8_t digivice_rules_charge_zone_w(uint16_t strength) {
    uint16_t capped = strength < DIGIVICE_STRENGTH_CAP ? strength : DIGIVICE_STRENGTH_CAP;
    return (uint8_t)(DIGIVICE_ZONE_BASE_W + capped / 8);
}

uint8_t digivice_rules_meter_cursor(uint32_t frame) {
    uint32_t period = 2 * (DIGIVICE_METER_SEGMENTS - 1);
    uint32_t q = frame % period;
    return (uint8_t)(q < DIGIVICE_METER_SEGMENTS ? q : period - q);
}

uint8_t digivice_rules_charge_zone_start(uint8_t zone_w, uint8_t rng) {
    // Starts in [3, 20 - w - 1]: never flush with either end of the meter.
    uint8_t span = (uint8_t)(DIGIVICE_METER_SEGMENTS - zone_w - 3);
    return (uint8_t)(3 + rng % span);
}

DigiviceCharge digivice_rules_charge_tier(uint8_t cursor, uint8_t zone_start, uint8_t zone_w) {
    int32_t c = cursor;
    int32_t lo = zone_start;
    int32_t hi = zone_start + zone_w; // exclusive
    if(c >= lo && c < hi) return DigiviceChargeBig;
    if(c >= lo - DIGIVICE_NEAR_SEGMENTS && c < hi + DIGIVICE_NEAR_SEGMENTS) {
        return DigiviceChargeNormal;
    }
    return DigiviceChargeSmall;
}

uint8_t digivice_rules_guard_at(uint8_t g0, uint32_t frame) {
    return (uint8_t)((g0 ^ (frame / DIGIVICE_GUARD_FLIP_FRAMES)) & 1);
}

bool digivice_rules_shot_blocked(uint8_t aim, uint8_t g0, uint32_t press_frame) {
    return aim == digivice_rules_guard_at(g0, press_frame + DIGIVICE_IMPACT_LAG_FRAMES);
}

void digivice_battle_init(
    DigiviceBattle* b,
    const DigiviceState* s,
    DigiviceEncounterKind kind,
    uint32_t now) {
    memset(b, 0, sizeof(DigiviceBattle));
    DigiviceStage stage = digivice_rules_stage(s);
    b->kind = kind;
    b->player_attr = digivice_rules_form_attr(s);
    b->player_power = digivice_rules_player_power(s, now);
    b->player_shot = stage_shot[stage];
    b->shield = s->item == DigiviceItemShield;
    b->glitchy = digivice_rules_glitchy(s);

    if(kind == DigiviceEncounterBoss) {
        b->foe = s->area;
        b->foe_attr = digivice_expedition_boss_attr(s->area);
        b->foe_power = digivice_expedition_boss_power(s->area);
        b->foe_shot = s->area == DIGIVICE_AREA_COUNT - 1 ? DIGIVICE_SHOT_BEAM : DIGIVICE_SHOT_BOSS;
    } else if(kind == DigiviceEncounterVirux) {
        b->foe = s->virux_type;
        b->foe_attr = digivice_rules_type_attr((DigiviceType)s->virux_type);
        b->foe_power = virux_power[stage];
        b->foe_shot = b->player_shot;
    } else {
        DigiviceFoe foe = digivice_expedition_pool_foe(s->area, s->enc_foe <= 1 ? s->enc_foe : 0);
        b->foe = foe;
        b->foe_attr = digivice_expedition_foe_attr(foe);
        b->foe_power = digivice_expedition_foe_power(s->area, foe);
        b->foe_shot = digivice_expedition_foe_champion(foe) ? DIGIVICE_SHOT_CHAMP_FOE :
                                                              DIGIVICE_SHOT_ROOKIE_FOE;
    }
}

DigiviceShot digivice_battle_player_shot(
    DigiviceBattle* b,
    DigiviceCharge tier,
    bool blocked,
    uint8_t miss_roll,
    uint8_t hit_roll) {
    uint8_t pct = digivice_rules_hit_pct(
        digivice_rules_charged_power(b->player_power, tier),
        b->foe_power,
        digivice_rules_attr_adv(b->player_attr, b->foe_attr));
    if(blocked) {
        pct /= 2;
        if(pct < DIGIVICE_BLOCK_MIN_PCT) pct = DIGIVICE_BLOCK_MIN_PCT;
    }
    bool corrupt_miss = b->glitchy && miss_roll < DIGIVICE_CORRUPTION_MISS_PCT;
    bool hit = !corrupt_miss && hit_roll < pct;
    if(hit) b->player_hits++;
    if(blocked) return hit ? DigiviceShotGuardBroken : DigiviceShotBlocked;
    return hit ? DigiviceShotHit : DigiviceShotDodge;
}

DigiviceShot digivice_battle_foe_shot(DigiviceBattle* b, uint8_t hit_roll) {
    uint8_t pct = digivice_rules_hit_pct(
        b->foe_power, b->player_power, digivice_rules_attr_adv(b->foe_attr, b->player_attr));
    DigiviceShot shot = DigiviceShotDodge;
    if(hit_roll < pct) {
        if(b->shield) {
            b->shield = false;
            b->shield_used = true;
            shot = DigiviceShotShielded;
        } else {
            b->foe_hits++;
            shot = DigiviceShotHit;
        }
    }
    b->round++;
    return shot;
}

DigiviceBattleOutcome digivice_battle_outcome(const DigiviceBattle* b) {
    if(b->player_hits >= DIGIVICE_HITS_TO_WIN) return DigiviceBattleWon;
    if(b->foe_hits >= DIGIVICE_HITS_TO_WIN) return DigiviceBattleLost;
    if(b->round < DIGIVICE_ROUND_CAP) return DigiviceBattleOngoing;
    if(b->player_hits > b->foe_hits) return DigiviceBattleWon;
    if(b->foe_hits > b->player_hits) return DigiviceBattleLost;
    return DigiviceBattleDrawn;
}

bool digivice_battle_retreat_allowed(const DigiviceState* s, DigiviceEncounterKind kind) {
    return kind == DigiviceEncounterVirux || !digivice_expedition_streak_area(s->area);
}

static void drop_shield(DigiviceState* s, DigiviceReward* reward) {
    if(s->item != DigiviceItemNone) return;
    s->item = DigiviceItemShield;
    reward->item = DigiviceItemShield;
}

DigiviceReward digivice_rules_battle_result(
    DigiviceState* s,
    DigiviceEncounterKind kind,
    DigiviceBattleResult result,
    bool shield_used,
    uint8_t rng_pct,
    uint32_t now) {
    DigiviceReward reward = {.exp = 0, .item = DigiviceItemNone};
    if(shield_used && s->item == DigiviceItemShield) s->item = DigiviceItemNone;
    uint8_t area = s->area < DIGIVICE_AREA_COUNT ? s->area : DIGIVICE_AREA_COUNT - 1;

    if(result == DigiviceBattleResultWin) {
        inc_u16(&s->wins);
        if(kind == DigiviceEncounterTrail) {
            reward.exp = DIGIVICE_TRAIL_EXP_BASE + DIGIVICE_TRAIL_EXP_AREA * (area + 1);
            if(rng_pct < DIGIVICE_DROP_PCT) drop_shield(s, &reward);
        } else if(kind == DigiviceEncounterBoss) {
            reward.exp = DIGIVICE_BOSS_EXP_BASE + DIGIVICE_BOSS_EXP_AREA * (area + 1);
            if(s->cleared & (1U << area)) reward.exp /= 2;
            drop_shield(s, &reward);
            digivice_expedition_boss_won(s);
        } else {
            reward.exp = 2 * DIGIVICE_WIN_BASE_EXP;
            if(s->virux_type < DigiviceTypeCount) {
                uint32_t data = (uint32_t)s->data[s->virux_type] + DIGIVICE_WIN_DATA;
                s->data[s->virux_type] = data > UINT16_MAX ? UINT16_MAX : (uint16_t)data;
            }
        }
        s->exp += reward.exp;
    } else if(result == DigiviceBattleResultLose) {
        inc_u16(&s->losses);
        digivice_rules_add_corruption(s, DIGIVICE_CORRUPTION_LOSE_GAIN, now);
        if(s->mood > 0) s->mood--;
        s->injured_until = now + DIGIVICE_INJURY_S;
        if(kind != DigiviceEncounterVirux) digivice_expedition_streak_lost(s);
    }

    if(kind == DigiviceEncounterTrail) {
        s->enc_foe = DIGIVICE_ENC_NONE;
        s->enc_expires = 0;
    } else if(kind == DigiviceEncounterVirux) {
        s->virux_type = DigiviceTypeNone;
    }
    return reward;
}
