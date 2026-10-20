#include "digivice_battle.h"
#include "digivice_config.h"
#include "digivice_city.h"

static const uint16_t stage_power[DigiviceStageCount] = DIGIVICE_STAGE_POWER;

static void inc_u16(uint16_t* value) {
    if(*value < UINT16_MAX) (*value)++;
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
    uint32_t offered_expires,
    uint8_t rng_pct,
    uint32_t now) {
    DigiviceReward reward = {.exp = 0, .item = DigiviceItemNone};
    bool homage = kind == DigiviceEncounterTrail && s->enc_expires == offered_expires &&
                  digivice_homage_code(s->enc_foe);
    bool city = kind == DigiviceEncounterTrail && s->enc_expires == offered_expires &&
                digivice_city_code(s->enc_foe);
    uint8_t partner = homage ? (uint8_t)(s->enc_foe - DIGIVICE_ENC_HOMAGE) : 0;
    if(result == DigiviceBattleResultWin || result == DigiviceBattleResultLose) {
        digivice_rules_count_battle(s, result == DigiviceBattleResultWin);
        if(!homage) digivice_homage_count_battle(s);
    }
    if(shield_used && s->item == DigiviceItemShield) s->item = DigiviceItemNone;
    uint8_t area = s->area < DIGIVICE_AREA_COUNT ? s->area : DIGIVICE_AREA_COUNT - 1;

    if(result == DigiviceBattleResultWin) {
        inc_u16(&s->wins);
        if(kind == DigiviceEncounterTrail) {
            reward.exp = DIGIVICE_TRAIL_EXP_BASE + DIGIVICE_TRAIL_EXP_AREA * (area + 1);
            if(rng_pct < DIGIVICE_DROP_PCT) drop_shield(s, &reward);
            if(homage || city) reward.exp *= DIGIVICE_SPECIAL_EXP_X;
            if(homage) {
                reward.partner = partner;
                reward.capture = digivice_homage_resolve(s, partner, digivice_rules_rng_pct(s));
                if(reward.capture == DigiviceCaptureFriend) reward.exp += DIGIVICE_FRIEND_EXP;
            }
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
        // A Baby has no defences yet: a loss hurts but does not corrupt.
        if(digivice_rules_stage(s) >= DigiviceStageRookie) {
            digivice_rules_add_corruption(s, DIGIVICE_CORRUPTION_LOSE_GAIN, now);
        }
        if(s->mood > 0) s->mood--;
        s->injured_until = now + DIGIVICE_INJURY_S;
        if(kind != DigiviceEncounterVirux) digivice_expedition_streak_lost(s);
    }

    if(homage && result != DigiviceBattleResultWin) s->special_count = DIGIVICE_HOMAGE_RETRY;

    if(kind == DigiviceEncounterTrail) {
        // A long fight may have outlived its encounter and a newer one spawned meanwhile.
        if(s->enc_expires == offered_expires) {
            s->enc_foe = DIGIVICE_ENC_NONE;
            s->enc_expires = 0;
        }
    } else if(kind == DigiviceEncounterVirux) {
        s->virux_type = DigiviceTypeNone;
    }
    return reward;
}
