#include "digivice_moves.h"

#define M(n, pw, ac, p, at, f, a) \
    {.name = n, .power = pw, .acc = ac, .pp = p, .attr = at, .fx = f, .arg = a}

static const DigiviceMove moves[DigiviceMoveCount] = {
    [DigiviceMoveZarpazo] = M("ZARPAZO", 40, 100, 25, DigiviceAttrNulo, DigiviceFxNone, 0),
    [DigiviceMoveMordisco] = M("MORDISCO", 50, 95, 20, DigiviceAttrNulo, DigiviceFxNone, 0),
    [DigiviceMoveEmbestida] = M("EMBESTIDA", 70, 90, 15, DigiviceAttrNulo, DigiviceFxNone, 0),
    [DigiviceMoveReparar] = M("REPARAR", 0, 0, 8, DigiviceAttrNulo, DigiviceFxHeal, 50),
    [DigiviceMoveEscudoDatos] =
        M("ESCUDO DATOS", 0, 0, 8, DigiviceAttrCifrado, DigiviceFxShield, 0),
    [DigiviceMoveSobrecarga] = M("SOBRECARGA", 0, 0, 5, DigiviceAttrNulo, DigiviceFxCharge, 0),
    [DigiviceMoveRafaga] = M("RAFAGA", 30, 100, 20, DigiviceAttrNulo, DigiviceFxQuick, 0),
    [DigiviceMoveGrito] = M("GRITO", 0, 100, 15, DigiviceAttrNulo, DigiviceFxAtkDown, 100),
    [DigiviceMoveFragmentar] =
        M("FRAGMENTAR", 0, 100, 15, DigiviceAttrNulo, DigiviceFxDefDown, 100),
    [DigiviceMoveLucha] = M("LUCHA", 35, 100, 0, DigiviceAttrNulo, DigiviceFxNone, 0),

    [DigiviceMoveKit + 0] = M("ONDA CHOQUE", 55, 95, 15, DigiviceAttrPortador, DigiviceFxNone, 0),
    [DigiviceMoveKit + 1] = M("TSUNAMI", 85, 85, 8, DigiviceAttrPortador, DigiviceFxNone, 0),
    [DigiviceMoveKit + 2] =
        M("OLA BAJA", 40, 100, 20, DigiviceAttrPortador, DigiviceFxDefDown, 30),
    [DigiviceMoveKit + 3] = M("RESACA", 45, 100, 15, DigiviceAttrPortador, DigiviceFxAtkDown, 30),

    [DigiviceMoveKit + 4] = M("PIXEL BEAM", 55, 100, 15, DigiviceAttrRuido, DigiviceFxNone, 0),
    [DigiviceMoveKit + 5] = M("TORMENTA", 90, 80, 5, DigiviceAttrRuido, DigiviceFxNone, 0),
    [DigiviceMoveKit + 6] = M("RAYO GLITCH", 65, 90, 10, DigiviceAttrRuido, DigiviceFxGlitch, 30),
    [DigiviceMoveKit + 7] = M("INTERFERIR", 40, 100, 20, DigiviceAttrRuido, DigiviceFxAtkDown, 30),

    [DigiviceMoveKit + 8] = M("CIFRAR", 55, 100, 15, DigiviceAttrCifrado, DigiviceFxNone, 0),
    [DigiviceMoveKit + 9] = M("HACKEO", 85, 85, 8, DigiviceAttrCifrado, DigiviceFxNone, 0),
    [DigiviceMoveKit + 10] =
        M("CLAVE ROTA", 45, 100, 15, DigiviceAttrCifrado, DigiviceFxDefDown, 40),
    [DigiviceMoveKit + 11] = M("BLOQUEO", 40, 100, 20, DigiviceAttrCifrado, DigiviceFxGlitch, 25),

    [DigiviceMoveKit + 12] = M("ONDA NULA", 55, 100, 15, DigiviceAttrNulo, DigiviceFxNone, 0),
    [DigiviceMoveKit + 13] = M("PULSO FINAL", 85, 85, 8, DigiviceAttrNulo, DigiviceFxNone, 0),
    [DigiviceMoveKit + 14] = M("CHISPAZO", 45, 100, 20, DigiviceAttrNulo, DigiviceFxGlitch, 20),
    [DigiviceMoveKit + 15] = M("FRENAZO", 45, 100, 20, DigiviceAttrNulo, DigiviceFxDefDown, 30),
};

#define UTIL_COUNT 6
static const uint8_t utility[UTIL_COUNT] = {
    DigiviceMoveReparar,
    DigiviceMoveEscudoDatos,
    DigiviceMoveSobrecarga,
    DigiviceMoveRafaga,
    DigiviceMoveGrito,
    DigiviceMoveFragmentar,
};

const DigiviceMove* digivice_move(uint8_t id) {
    return &moves[id < DigiviceMoveCount ? id : DigiviceMoveLucha];
}

void digivice_moves_assign(
    uint8_t tier,
    uint8_t attr,
    uint8_t seed,
    uint8_t force_util,
    uint8_t out[4]) {
    if(attr >= DigiviceAttrCount) attr = DigiviceAttrNulo;
    if(tier > 4) tier = 4;
    uint8_t kit = (uint8_t)(DigiviceMoveKit + attr * 4);
    out[0] = tier == 0 ? DigiviceMoveZarpazo :
             tier >= 3 ? DigiviceMoveEmbestida :
                         DigiviceMoveMordisco;
    out[1] = (uint8_t)(kit + (tier >= 3 ? 1 : 0));
    out[2] = (uint8_t)(kit + 2 + (seed & 1));
    out[3] = force_util != DIGIVICE_MOVE_NONE ? force_util : utility[(seed + tier) % UTIL_COUNT];
}
