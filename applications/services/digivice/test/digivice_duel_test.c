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

int main(void) {
    test_tier_and_stats();
    test_damage();
    test_pick();
    if(failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("digivice duel: all passed\n");
    return 0;
}
