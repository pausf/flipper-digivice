// Host-side test for the move table. Run: applications/services/digivice/test/run.sh
#include "../digivice_config.h"
#include "../digivice_moves.h"

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

static void test_table(void) {
    CHECK(DigiviceMoveCount == 26);
    for(uint8_t id = 0; id < DigiviceMoveCount; id++) {
        const DigiviceMove* m = digivice_move(id);
        size_t len = strlen(m->name);
        CHECK(len >= 4 && len <= 12);
        for(size_t i = 0; i < len; i++) {
            CHECK(m->name[i] == ' ' || (m->name[i] >= 'A' && m->name[i] <= 'Z'));
        }
        CHECK(m->power <= 100 && m->acc <= 100);
        CHECK(m->attr < DigiviceAttrCount && m->fx < DigiviceFxCount);
        CHECK(id == DigiviceMoveLucha || m->pp >= 5);
        CHECK(m->power > 0 || m->fx != DigiviceFxNone);
        for(uint8_t j = 0; j < id; j++) CHECK(strcmp(digivice_move(j)->name, m->name) != 0);
    }
    CHECK(strcmp(digivice_move(200)->name, "LUCHA") == 0);
    // Kit layout: mid, heavy, effect A, effect B of the attribute.
    for(uint8_t attr = 0; attr < DigiviceAttrCount; attr++) {
        for(uint8_t k = 0; k < 4; k++) {
            const DigiviceMove* m = digivice_move((uint8_t)(DigiviceMoveKit + attr * 4 + k));
            CHECK(m->attr == attr && m->power > 0);
            CHECK((k < 2) == (m->fx == DigiviceFxNone));
        }
    }
}

static void test_assign(void) {
    uint8_t out[4];
    digivice_moves_assign(0, DigiviceAttrNulo, 0, DIGIVICE_MOVE_NONE, out);
    CHECK(out[0] == DigiviceMoveZarpazo && out[3] == DigiviceMoveReparar);
    CHECK(strcmp(digivice_move(out[1])->name, "ONDA NULA") == 0);
    CHECK(strcmp(digivice_move(out[2])->name, "CHISPAZO") == 0);
    digivice_moves_assign(1, DigiviceAttrCifrado, 1, DIGIVICE_MOVE_NONE, out);
    CHECK(out[0] == DigiviceMoveMordisco && out[3] == DigiviceMoveSobrecarga); // (1 + 1) % 6
    CHECK(strcmp(digivice_move(out[2])->name, "BLOQUEO") == 0); // odd seed: effect B
    digivice_moves_assign(4, DigiviceAttrRuido, 3, DIGIVICE_MOVE_NONE, out);
    CHECK(out[0] == DigiviceMoveEmbestida && out[3] == DigiviceMoveEscudoDatos);
    CHECK(strcmp(digivice_move(out[1])->name, "TORMENTA") == 0);
    digivice_moves_assign(2, DigiviceAttrPortador, 0, DigiviceMoveSobrecarga, out);
    CHECK(out[3] == DigiviceMoveSobrecarga);
    // Every tier, attribute and seed gets three damaging moves and never LUCHA.
    for(uint8_t tier = 0; tier <= 5; tier++) {
        for(uint8_t attr = 0; attr <= DigiviceAttrCount; attr++) {
            for(uint16_t seed = 0; seed < 64; seed++) {
                digivice_moves_assign(tier, attr, (uint8_t)seed, DIGIVICE_MOVE_NONE, out);
                uint8_t damaging = 0;
                for(int i = 0; i < 4; i++) {
                    CHECK(out[i] < DigiviceMoveCount && out[i] != DigiviceMoveLucha);
                    damaging += digivice_move(out[i])->power > 0;
                }
                CHECK(damaging >= 3);
            }
        }
    }
}

int main(void) {
    test_table();
    test_assign();
    if(failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("digivice moves: all passed\n");
    return 0;
}
