// Host-side test for the duel rules. Run: applications/services/digivice/test/run.sh
#include "../digivice_city.h"
#include "../digivice_config.h"
#include "../digivice_duel.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                \
    do {                                                           \
        if(!(cond)) {                                              \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                            \
        }                                                          \
    } while(0)

#define T0 (10UL * 86400UL + 8UL * 3600UL) // day 10 at 08:00

// A Champion-tier fighter of power `power`, shot size 5, plain moves.
static DigiviceFighter fighter(uint16_t power, uint8_t attr) {
    DigiviceFighter f;
    DigiviceFighterSpec spec = {
        .power = power, .attr = attr, .tier = 2, .force_util = DIGIVICE_MOVE_NONE, .shot = 5};
    digivice_fighter_init(&f, &spec);
    return f;
}

static void set_moves(DigiviceFighter* f, uint8_t a, uint8_t b, uint8_t c, uint8_t e) {
    f->moves[0] = a;
    f->moves[1] = b;
    f->moves[2] = c;
    f->moves[3] = e;
    for(int i = 0; i < DIGIVICE_SLOTS; i++) f->pp[i] = digivice_move(f->moves[i])->pp;
}

static void test_tier_and_stats(void) {
    CHECK(digivice_duel_tier(DigiviceStageBaby2) == 0);
    CHECK(digivice_duel_tier(DigiviceStageRookie) == 1);
    CHECK(digivice_duel_tier(DigiviceStageMega) == 4);
    DigiviceFighter f = fighter(40, DigiviceAttrRuido);
    CHECK(f.level == 26 && f.hp == 62 && f.hp_max == 62);
    CHECK(f.atk == 140 && f.def == 140 && f.spd == 30 && f.shot == 5);
    CHECK(f.pp[0] == digivice_move(f.moves[0])->pp);
    f = fighter(131, DigiviceAttrNulo); // an area 8 boss at full power
    CHECK(f.level == 41 && f.hp_max == 92 && f.atk == 231);
    DigiviceFighterSpec spec = {
        .power = 64, .attr = DigiviceAttrNulo, .force_util = DIGIVICE_MOVE_NONE, .spd_bonus = 4};
    digivice_fighter_init(&f, &spec);
    CHECK(f.spd == 10 + 32 + 4);
}

static void test_damage(void) {
    // Level 26, ATK 140 against DEF 140: (2 * 26 / 5 + 2) * 55 * 140 / 140 / 50 + 2 = 15.
    DigiviceFighter a = fighter(40, DigiviceAttrCifrado);
    DigiviceFighter d = fighter(40, DigiviceAttrNulo);
    const DigiviceMove* neutral = digivice_move(DigiviceMoveKit + 12); // ONDA NULA, Nulo 55
    const DigiviceMove* wave = digivice_move(DigiviceMoveKit + 0); // ONDA CHOQUE, Portador 55
    CHECK(digivice_duel_damage(&a, &d, neutral, false, 99) == 15);
    CHECK(digivice_duel_damage(&a, &d, neutral, false, 0) == 12); // the 85% roll
    CHECK(digivice_duel_damage(&a, &d, neutral, true, 99) == 22); // critical x1.5
    // Triangle: Portador beats Ruido (x1.5) and loses to Cifrado (x0.67).
    d.attr = DigiviceAttrRuido;
    CHECK(digivice_duel_damage(&a, &d, wave, false, 99) == 22);
    d.attr = DigiviceAttrCifrado;
    CHECK(digivice_duel_damage(&a, &d, wave, false, 99) == 10);
    d.attr = DigiviceAttrNulo;
    a.attr = DigiviceAttrPortador; // STAB x1.2
    CHECK(digivice_duel_damage(&a, &d, wave, false, 99) == 18);
    // One lowered ATK step is x0.80; two lowered DEF steps divide by x0.64.
    a.attr = DigiviceAttrCifrado;
    a.atk_low = 1;
    CHECK(digivice_duel_damage(&a, &d, neutral, false, 99) == 12);
    a.atk_low = 0;
    d.def_low = 2;
    CHECK(digivice_duel_damage(&a, &d, neutral, false, 99) == 22);
    // The weakest fighter always scratches the toughest one for at least 1, never a KO.
    DigiviceFighter weak = fighter(0, DigiviceAttrNulo);
    DigiviceFighter tank = fighter(131, DigiviceAttrCifrado);
    for(uint8_t id = 0; id < DigiviceMoveCount; id++) {
        const DigiviceMove* m = digivice_move(id);
        if(!m->power) continue;
        CHECK(digivice_duel_damage(&weak, &tank, m, false, 0) >= 1);
        CHECK(digivice_duel_damage(&weak, &tank, m, true, 99) < tank.hp_max);
    }
}

static int picks(const DigiviceFighter* me, const DigiviceFighter* foe, uint8_t slot) {
    int n = 0;
    for(int roll = 0; roll < 256; roll++) {
        n += digivice_fighter_pick(me, foe, (uint8_t)roll) == slot;
    }
    return n;
}

static void test_pick(void) {
    // Portador, Ruido, Cifrado, Nulo against a Ruido foe: x2, x1, x0.5 and x1 weights.
    DigiviceFighter me = fighter(40, DigiviceAttrNulo);
    set_moves(
        &me, DigiviceMoveKit + 0, DigiviceMoveKit + 4, DigiviceMoveKit + 8, DigiviceMoveKit + 12);
    DigiviceFighter foe = fighter(40, DigiviceAttrRuido);
    CHECK(picks(&me, &foe, 0) > picks(&me, &foe, 1));
    CHECK(picks(&me, &foe, 2) > 0 && picks(&me, &foe, 2) < picks(&me, &foe, 3));

    // Heals when low (and only then); a spent move is never picked; all spent is LUCHA.
    set_moves(
        &me, DigiviceMoveZarpazo, DigiviceMoveMordisco, DigiviceMoveReparar, DigiviceMoveRafaga);
    CHECK(picks(&me, &foe, 2) == 0);
    me.hp = me.hp_max * 30 / 100;
    CHECK(picks(&me, &foe, 2) > 128);
    me.hp = me.hp_max * 50 / 100;
    CHECK(picks(&me, &foe, 2) == 0);
    me.pp[0] = 0;
    CHECK(picks(&me, &foe, 0) == 0);
    memset(me.pp, 0, sizeof(me.pp));
    CHECK(digivice_fighter_spent(&me));
    CHECK(picks(&me, &foe, DIGIVICE_SLOT_STRUGGLE) == 256);

    // A shield already up and lowerings already maxed are not worth a turn.
    set_moves(
        &me,
        DigiviceMoveEscudoDatos,
        DigiviceMoveSobrecarga,
        DigiviceMoveGrito,
        DigiviceMoveFragmentar);
    me.guard = me.charged = true;
    foe.atk_low = foe.def_low = DIGIVICE_STAGE_MAX;
    CHECK(picks(&me, &foe, DIGIVICE_SLOT_STRUGGLE) == 256);
}

static DigiviceState with_form(uint8_t form) {
    DigiviceState s;
    digivice_rules_init(&s, T0);
    s.form = form;
    return s;
}

static void test_init(void) {
    DigiviceState s = with_form(DigiviceFormAriete); // Champion, Portador
    DigiviceDuel d;
    digivice_duel_init(&d, &s, DigiviceEncounterTrail, T0);
    const DigiviceFighter* pet = &d.f[DigiviceSidePlayer];
    CHECK(pet->attr == DigiviceAttrPortador && pet->level == 26 && pet->hp_max == 62);
    CHECK(pet->shot == 5 && !d.item_shield && !d.glitchy && d.round == 0);

    // A trail foe is built from the area power, scaled by DIGIVICE_FOE_POWER_PCT.
    s.area = 3;
    s.enc_foe = 1;
    digivice_duel_init(&d, &s, DigiviceEncounterTrail, T0);
    DigiviceFoe foe = digivice_expedition_pool_foe(3, 1);
    uint16_t power = digivice_expedition_foe_power(3, foe) * DIGIVICE_FOE_POWER_PCT / 100;
    const DigiviceFighter* f = &d.f[DigiviceSideFoe];
    CHECK(d.foe == foe && d.foe_class == DigiviceFoeClassPool);
    CHECK(f->attr == digivice_expedition_foe_attr(foe));
    CHECK(f->level == DIGIVICE_LEVEL_BASE + power / DIGIVICE_LEVEL_DIV);

    // Injury (x3/4 power), the held Shield, corruption and vigor all reach the pet.
    s.injured_until = T0 + 1;
    s.item = DigiviceItemShield;
    s.corruption = DIGIVICE_CORRUPTION_GLITCH;
    s.vigor = 3;
    digivice_duel_init(&d, &s, DigiviceEncounterTrail, T0);
    CHECK(d.f[0].level == 25 && d.f[0].spd == 10 + 15 + 6);
    CHECK(d.item_shield && d.glitchy);
}

static void test_init_special_foes(void) {
    DigiviceState s = with_form(DigiviceFormAriete);
    DigiviceDuel d;
    for(uint8_t area = 0; area < DIGIVICE_AREA_COUNT; area++) {
        s.area = area;
        digivice_duel_init(&d, &s, DigiviceEncounterBoss, T0);
        const DigiviceFighter* b = &d.f[DigiviceSideFoe];
        CHECK(d.foe == area && b->attr == digivice_expedition_boss_attr(area));
        CHECK(b->moves[3] == (area & 1 ? DigiviceMoveSobrecarga : DigiviceMoveEscudoDatos));
        CHECK(b->moves[0] == (area >= 3 ? DigiviceMoveEmbestida : DigiviceMoveMordisco));
    }
    s.area = 0;
    s.virux_type = DigiviceTypeIr;
    digivice_duel_init(&d, &s, DigiviceEncounterVirux, T0);
    CHECK(d.f[1].moves[3] == DigiviceMoveSobrecarga);
    CHECK(d.f[1].attr == digivice_rules_type_attr(DigiviceTypeIr));

    s.enc_foe = DIGIVICE_ENC_CITY + 2;
    digivice_duel_init(&d, &s, DigiviceEncounterTrail, T0);
    CHECK(d.foe_class == DigiviceFoeClassCity && d.foe == 2);
    CHECK(d.f[1].attr == digivice_city_monster_attr(2) && d.f[1].shot == DIGIVICE_SHOT_CHAMP_FOE);
#if DIGIVICE_EASTER_EGGS
    s.enc_foe = DIGIVICE_ENC_HOMAGE + 3; // 40 x 1.2 = 48: level 28, not scaled down
    digivice_duel_init(&d, &s, DigiviceEncounterTrail, T0);
    CHECK(d.foe_class == DigiviceFoeClassHomage && d.foe == 3 && d.f[1].level == 28);
    CHECK(d.f[1].attr == digivice_homage_attr(3));
#endif
}

static DigiviceDuel arena(void) {
    DigiviceDuel d;
    memset(&d, 0, sizeof(d));
    d.f[DigiviceSidePlayer] = fighter(40, DigiviceAttrNulo);
    d.f[DigiviceSideFoe] = fighter(40, DigiviceAttrNulo);
    return d;
}

// Hit, best damage roll, no critical hit, no side effect, no paralysis, no corruption fail.
static DigiviceRolls plain(void) {
    DigiviceRolls r = {.acc = 0, .var = 99, .crit = 1, .fx = 99, .para = 99, .miss = 99};
    return r;
}

static DigiviceStep act(DigiviceDuel* d, uint8_t side, uint8_t slot, const DigiviceRolls* r) {
    return digivice_duel_act(d, side, slot, false, r);
}

static void test_act_damage(void) {
    DigiviceDuel d = arena();
    DigiviceRolls r = plain();
    set_moves(&d.f[0], DigiviceMoveMordisco, DigiviceMoveZarpazo, DigiviceMoveReparar, 0);
    // 12 * 50 * 140 / 140 / 50 + 2 = 14, x1.2 STAB (a Nulo move from a Nulo user) = 16.
    DigiviceStep st = act(&d, DigiviceSidePlayer, 0, &r);
    CHECK(st.result == DigiviceStepDone && st.damage == 16 && st.edge == 0 && !st.crit);
    CHECK(st.move == DigiviceMoveMordisco && d.f[1].hp == 62 - 16 && d.f[0].pp[0] == 19);
    // MORDISCO is 95% accurate: roll 95 misses, still costs the PP and does nothing.
    r.acc = 95;
    st = act(&d, DigiviceSidePlayer, 0, &r);
    CHECK(st.result == DigiviceStepMissed && st.damage == 0 && d.f[1].hp == 46);
    CHECK(d.f[0].pp[0] == 18);
    // 1 in 16 is critical, and the timing bar guarantees one: x1.5.
    r = plain();
    r.crit = 32;
    CHECK(act(&d, DigiviceSidePlayer, 1, &r).crit);
    r.crit = 33;
    CHECK(!act(&d, DigiviceSidePlayer, 1, &r).crit);
    CHECK(digivice_duel_act(&d, DigiviceSidePlayer, 1, true, &r).crit);
    // The step reports the triangle edge for the UI messages.
    d.f[1].attr = DigiviceAttrRuido;
    set_moves(&d.f[0], DigiviceMoveKit + 0, DigiviceMoveKit + 8, DigiviceMoveZarpazo, 0);
    CHECK(act(&d, DigiviceSidePlayer, 0, &r).edge == 1);
    CHECK(act(&d, DigiviceSidePlayer, 1, &r).edge == -1);
    CHECK(act(&d, DigiviceSidePlayer, 2, &r).edge == 0);
    // No PP left: LUCHA, which costs nothing.
    d.f[1].hp = d.f[1].hp_max;
    memset(d.f[0].pp, 0, sizeof(d.f[0].pp));
    st = act(&d, DigiviceSidePlayer, 2, &r);
    CHECK(st.slot == DIGIVICE_SLOT_STRUGGLE && st.move == DigiviceMoveLucha && st.damage > 0);
}

static void test_act_status_moves(void) {
    DigiviceDuel d = arena();
    DigiviceRolls r = plain();
    set_moves(
        &d.f[0],
        DigiviceMoveSobrecarga,
        DigiviceMoveZarpazo,
        DigiviceMoveReparar,
        DigiviceMoveEscudoDatos);
    set_moves(&d.f[1], DigiviceMoveGrito, DigiviceMoveFragmentar, DigiviceMoveZarpazo, 0);
    // SOBRECARGA: nothing now, the next damaging move deals x2 once. ZARPAZO: (11 + STAB) = 13.
    CHECK(act(&d, 0, 0, &r).fx == DigiviceFxCharge && d.f[0].charged);
    CHECK(act(&d, 0, 1, &r).damage == 26 && !d.f[0].charged);
    // GRITO and FRAGMENTAR lower the foe's ATK and DEF up to two steps.
    CHECK(act(&d, 1, 0, &r).fx == DigiviceFxAtkDown);
    CHECK(act(&d, 1, 0, &r).fx == DigiviceFxAtkDown);
    CHECK(act(&d, 1, 0, &r).fx == DigiviceFxNone && d.f[0].atk_low == DIGIVICE_STAGE_MAX);
    CHECK(act(&d, 1, 1, &r).fx == DigiviceFxDefDown && d.f[0].def_low == 1);
    // ESCUDO DATOS halves the next damage once, rounding up.
    CHECK(act(&d, 0, 3, &r).fx == DigiviceFxShield && d.f[0].guard);
    uint16_t hp = d.f[0].hp;
    DigiviceStep st = act(&d, 1, 2, &r);
    CHECK(!d.f[0].guard && st.damage > 0 && hp - d.f[0].hp == st.damage);
    // REPARAR heals half the max HP, never past it.
    d.f[0].hp = 10;
    st = act(&d, 0, 2, &r);
    CHECK(st.fx == DigiviceFxHeal && st.healed == 31 && d.f[0].hp == 41);
    st = act(&d, 0, 2, &r);
    CHECK(st.healed == 21 && d.f[0].hp == 62);
}

static void test_glitch_and_order(void) {
    DigiviceDuel d = arena();
    DigiviceRolls r = plain();
    set_moves(&d.f[0], DigiviceMoveKit + 6, DigiviceMoveRafaga, DigiviceMoveZarpazo, 0);
    set_moves(&d.f[1], DigiviceMoveZarpazo, DigiviceMoveRafaga, DigiviceMoveZarpazo, 0);
    r.fx = 30; // RAYO GLITCH lands below 30
    CHECK(act(&d, 0, 0, &r).fx == DigiviceFxNone && !d.f[1].glitch);
    r.fx = 29;
    r.acc = 89;
    CHECK(act(&d, 0, 0, &r).fx == DigiviceFxGlitch && d.f[1].glitch);
    // A glitched fighter loses 25% of its turns without spending PP.
    r.para = 24;
    DigiviceStep st = act(&d, 1, 0, &r);
    CHECK(st.result == DigiviceStepSkipped && st.damage == 0 && d.f[1].pp[0] == 25);
    r.para = 25;
    CHECK(act(&d, 1, 0, &r).result == DigiviceStepDone);

    // Order: priority first, then speed (halved while glitched), the pet on a tie.
    d.f[1].glitch = false;
    d.f[0].spd = d.f[1].spd = 30;
    CHECK(digivice_duel_player_first(&d, 0, 0));
    d.f[1].spd = 31;
    CHECK(!digivice_duel_player_first(&d, 0, 0));
    CHECK(digivice_duel_player_first(&d, 1, 0)); // RAFAGA has priority
    CHECK(!digivice_duel_player_first(&d, 1, 1)); // both quick: speed again
    d.f[1].glitch = true; // 31 / 2 < 30
    CHECK(digivice_duel_player_first(&d, 0, 0));
}

static void test_shield_item_and_corruption(void) {
    DigiviceDuel d = arena();
    DigiviceRolls r = plain();
    set_moves(&d.f[0], DigiviceMoveZarpazo, DigiviceMoveZarpazo, DigiviceMoveZarpazo, 0);
    set_moves(&d.f[1], DigiviceMoveZarpazo, DigiviceMoveZarpazo, DigiviceMoveZarpazo, 0);
    CHECK(!digivice_duel_shield_ready(&d) && !digivice_duel_arm_shield(&d));
    d.item_shield = true;
    CHECK(act(&d, 1, 0, &r).damage > 0 && !d.shield_used); // held, not armed: no effect
    CHECK(digivice_duel_arm_shield(&d) && !digivice_duel_arm_shield(&d));
    uint16_t hp = d.f[0].hp;
    DigiviceStep st = act(&d, 1, 1, &r);
    CHECK(st.result == DigiviceStepShielded && d.f[0].hp == hp && d.shield_used);
    CHECK(act(&d, 1, 2, &r).damage > 0); // only the first hit is cancelled

    // Corruption: 25% of the pet's moves fail, the foe's never do.
    d.glitchy = true;
    r.miss = DIGIVICE_CORRUPTION_MISS_PCT - 1;
    CHECK(act(&d, 0, 0, &r).result == DigiviceStepMissed);
    CHECK(act(&d, 1, 0, &r).result == DigiviceStepDone);
    r.miss = DIGIVICE_CORRUPTION_MISS_PCT;
    CHECK(act(&d, 0, 0, &r).result == DigiviceStepDone);
}

static void test_outcome(void) {
    DigiviceDuel d = arena();
    CHECK(digivice_duel_outcome(&d) == DigiviceBattleOngoing);
    d.f[1].hp = 0;
    CHECK(digivice_duel_outcome(&d) == DigiviceBattleWon);
    d.f[1].hp = 1;
    d.f[0].hp = 0;
    CHECK(digivice_duel_outcome(&d) == DigiviceBattleLost);
    // After the round cap the higher HP fraction wins, whatever the max HP.
    d = arena();
    d.f[0] = fighter(94, DigiviceAttrNulo); // 80 max
    for(int i = 0; i < DIGIVICE_TURN_CAP; i++) {
        CHECK(digivice_duel_outcome(&d) == DigiviceBattleOngoing);
        digivice_duel_end_round(&d);
    }
    CHECK(digivice_duel_outcome(&d) == DigiviceBattleDrawn); // both full
    d.f[0].hp = 40; // 50% against 48%
    d.f[1].hp = 30;
    CHECK(digivice_duel_outcome(&d) == DigiviceBattleWon);
    d.f[1].hp = 32; // 51%
    CHECK(digivice_duel_outcome(&d) == DigiviceBattleLost);
}

static uint32_t lcg(uint32_t* seed) {
    *seed = *seed * 1664525UL + 1013904223UL;
    return *seed >> 8;
}

// Both sides play the foe AI with random dice; the duel must end within the round cap.
static DigiviceBattleOutcome play_out(DigiviceDuel* d, uint32_t* seed) {
    while(digivice_duel_outcome(d) == DigiviceBattleOngoing) {
        uint8_t ps = digivice_fighter_pick(&d->f[0], &d->f[1], (uint8_t)lcg(seed));
        uint8_t fs = digivice_fighter_pick(&d->f[1], &d->f[0], (uint8_t)lcg(seed));
        bool pfirst = digivice_duel_player_first(d, ps, fs);
        for(int turn = 0; turn < 2; turn++) {
            bool player = (turn == 0) == pfirst;
            DigiviceRolls r = {
                .acc = (uint8_t)(lcg(seed) % 100),
                .var = (uint8_t)(lcg(seed) % 100),
                .crit = (uint8_t)(lcg(seed) % 256),
                .fx = (uint8_t)(lcg(seed) % 100),
                .para = (uint8_t)(lcg(seed) % 100),
                .miss = (uint8_t)(lcg(seed) % 100),
            };
            act(d, player ? DigiviceSidePlayer : DigiviceSideFoe, player ? ps : fs, &r);
            if(d->f[0].hp == 0 || d->f[1].hp == 0) break;
        }
        digivice_duel_end_round(d);
        if(d->round > DIGIVICE_TURN_CAP) return DigiviceBattleOngoing;
    }
    return digivice_duel_outcome(d);
}

static int win_percent(uint16_t p, uint8_t pa, uint16_t q, uint8_t qa) {
    int wins = 0;
    uint32_t seed = 12345;
    for(int i = 0; i < 400; i++) {
        DigiviceDuel d = arena();
        d.f[0] = fighter(p, pa);
        d.f[1] = fighter(q, qa);
        DigiviceBattleOutcome o = play_out(&d, &seed);
        CHECK(o != DigiviceBattleOngoing);
        wins += o == DigiviceBattleWon;
    }
    return wins / 4;
}

static void test_simulated_balance(void) {
    // Orderings only (the absolute numbers are tuned on the device): a mirror favours the
    // first mover without being a lock, the triangle and the power gap both matter.
    int mirror = win_percent(40, DigiviceAttrNulo, 40, DigiviceAttrNulo);
    CHECK(mirror >= 40 && mirror <= 90);
    CHECK(win_percent(40, DigiviceAttrPortador, 40, DigiviceAttrRuido) >
          win_percent(40, DigiviceAttrRuido, 40, DigiviceAttrPortador) + 20);
    CHECK(win_percent(20, DigiviceAttrNulo, 60, DigiviceAttrNulo) < 20);
}

static void test_duel_feeds_battle_result(void) {
    DigiviceState s = with_form(DigiviceFormAriete);
    s.item = DigiviceItemShield;
    s.area = 1;
    s.enc_foe = 0;
    s.enc_expires = T0 + 100;
    DigiviceDuel d;
    digivice_duel_init(&d, &s, DigiviceEncounterTrail, T0);
    DigiviceRolls r = plain();
    CHECK(digivice_duel_arm_shield(&d));
    d.f[1].hp = 1; // one hit from the end
    CHECK(act(&d, DigiviceSideFoe, 0, &r).result == DigiviceStepShielded);
    CHECK(act(&d, DigiviceSidePlayer, 0, &r).result == DigiviceStepDone);
    CHECK(digivice_duel_outcome(&d) == DigiviceBattleWon);
    DigiviceReward reward = digivice_rules_battle_result(
        &s, DigiviceEncounterTrail, DigiviceBattleResultWin, d.shield_used, T0 + 100, 99, T0);
    CHECK(reward.exp > 0 && s.wins == 1 && s.exp == reward.exp);
    CHECK(s.item == DigiviceItemNone && s.enc_foe == DIGIVICE_ENC_NONE); // the Shield is spent

    // An armed Shield that never met a foe hit is kept.
    s = with_form(DigiviceFormAriete);
    s.item = DigiviceItemShield;
    digivice_duel_init(&d, &s, DigiviceEncounterTrail, T0);
    digivice_duel_arm_shield(&d);
    digivice_rules_battle_result(
        &s, DigiviceEncounterTrail, DigiviceBattleResultFled, d.shield_used, 0, 99, T0);
    CHECK(s.item == DigiviceItemShield);
}

int main(void) {
    test_tier_and_stats();
    test_damage();
    test_pick();
    test_init();
    test_init_special_foes();
    test_act_damage();
    test_act_status_moves();
    test_glitch_and_order();
    test_shield_item_and_corruption();
    test_outcome();
    test_simulated_balance();
    test_duel_feeds_battle_result();
    if(failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("digivice duel: all passed\n");
    return 0;
}
