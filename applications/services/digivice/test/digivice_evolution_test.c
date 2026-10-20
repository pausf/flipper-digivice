// Host-side test for the evolution tree. Run: applications/services/digivice/test/run.sh
#include "../digivice_evolution.h"
#include "../digivice_config.h"
#include "../digivice_roster.h"

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

#define T0 1000000UL

// A pet that has been `form` for exactly its full time in stage.
static DigiviceState ready(uint8_t form) {
    DigiviceState s;
    digivice_rules_init(&s, T0);
    s.form = form;
    s.evo_from = DigiviceFormNone;
    s.stage_since = T0;
    return s;
}

static uint32_t due(uint8_t form) {
    return T0 + digivice_roster(form)->stay_s;
}

static void test_nothing_before_the_timer(void) {
    DigiviceState s = ready(DigiviceFormPipo);
    CHECK(digivice_evolution_next(&s, due(DigiviceFormPipo) - 1) == DigiviceFormNone);
    CHECK(digivice_evolution_next(&s, due(DigiviceFormPipo)) == DigiviceFormOrejin);
}

static void test_babies_follow_their_line(void) {
    DigiviceState a = ready(DigiviceFormChispi);
    CHECK(digivice_evolution_next(&a, due(DigiviceFormChispi)) == DigiviceFormColix);
    DigiviceState b = ready(DigiviceFormOrejin);
    CHECK(digivice_evolution_next(&b, due(DigiviceFormOrejin)) == DigiviceFormKorno);
    b.evo_mistakes = 3;
    CHECK(digivice_evolution_next(&b, due(DigiviceFormOrejin)) == DigiviceFormEstatik);
    DigiviceState c = ready(DigiviceFormColix);
    c.evo_mistakes = 2;
    CHECK(digivice_evolution_next(&c, due(DigiviceFormColix)) == DigiviceFormChipko);
    c.evo_mistakes = 3;
    CHECK(digivice_evolution_next(&c, due(DigiviceFormColix)) == DigiviceFormMokobit);
}

static void test_korno_branches_in_the_listed_order(void) {
    DigiviceState s = ready(DigiviceFormKorno);
    uint32_t at = due(DigiviceFormKorno);
    s.train_count = 24;
    s.evo_mistakes = 1;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormAriete);
    s.train_count = 12;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormEcolobo);
    s.train_count = 23;
    s.evo_mistakes = 4; // 12 to 23 trainings win over the mistakes branch
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormEcolobo);
    s.train_count = 24;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormVampixel);
    s.train_count = 3;
    s.evo_mistakes = 0;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormCharco);
}

static void test_other_rookies(void) {
    DigiviceState e = ready(DigiviceFormEstatik);
    uint32_t at = due(DigiviceFormEstatik);
    e.train_count = 16;
    CHECK(digivice_evolution_next(&e, at) == DigiviceFormFuzzcan);
    e.train_count = 0;
    e.overfeed = 3;
    CHECK(digivice_evolution_next(&e, at) == DigiviceFormSilbapo);
    e.overfeed = 2;
    CHECK(digivice_evolution_next(&e, at) == DigiviceFormCharco);

    DigiviceState c = ready(DigiviceFormChipko);
    c.train_count = 24;
    CHECK(digivice_evolution_next(&c, at) == DigiviceFormCerrojo);
    c.train_count = 23;
    c.night_owl = 3;
    CHECK(digivice_evolution_next(&c, at) == DigiviceFormBuhoscan);
    c.night_owl = 2;
    CHECK(digivice_evolution_next(&c, at) == DigiviceFormCharco);

    DigiviceState m = ready(DigiviceFormMokobit);
    m.train_count = 16;
    CHECK(digivice_evolution_next(&m, at) == DigiviceFormChatarro);
    m.train_count = 15;
    CHECK(digivice_evolution_next(&m, at) == DigiviceFormCharco);
}

static void test_champions_need_the_battle_record(void) {
    const uint8_t from[] = {
        DigiviceFormAriete, DigiviceFormEcolobo,  DigiviceFormVampixel, DigiviceFormFuzzcan,
        DigiviceFormSilbapo, DigiviceFormCharco,  DigiviceFormCerrojo,  DigiviceFormBuhoscan,
        DigiviceFormChatarro,
    };
    const uint8_t to[] = {
        DigiviceFormLanzero, DigiviceFormLanzero, DigiviceFormWyvolt,  DigiviceFormWyvolt,
        DigiviceFormFangorey, DigiviceFormFangorey, DigiviceFormBovedax, DigiviceFormBovedax,
        DigiviceFormKoloso,
    };
    for(size_t i = 0; i < sizeof(from); i++) {
        DigiviceState s = ready(from[i]);
        uint32_t at = due(from[i]);
        s.battles = 12;
        s.wins_stage = 7; // 58%
        CHECK(digivice_evolution_next(&s, at) == DigiviceFormNone);
        s.wins_stage = 8; // 66%
        CHECK(digivice_evolution_next(&s, at) == to[i]);
        s.battles = 11;
        CHECK(digivice_evolution_next(&s, at) == DigiviceFormNone);
    }
}

static void test_mega_needs_few_mistakes_and_a_strong_record(void) {
    DigiviceState s = ready(DigiviceFormLanzero);
    uint32_t at = due(DigiviceFormLanzero);
    s.battles = 10;
    s.wins_stage = 8;
    s.evo_mistakes = 1;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormAureola);
    s.evo_mistakes = 2;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormNone);
    s.evo_mistakes = 0;
    s.wins_stage = 7;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormNone);

    const uint8_t from[] = {DigiviceFormWyvolt, DigiviceFormBovedax, DigiviceFormKoloso};
    const uint8_t to[] = {DigiviceFormDrakstat, DigiviceFormOrakulo, DigiviceFormNularca};
    for(size_t i = 0; i < sizeof(from); i++) {
        DigiviceState t = ready(from[i]);
        t.battles = 10;
        t.wins_stage = 9;
        CHECK(digivice_evolution_next(&t, due(from[i])) == to[i]);
    }
}

static void test_fangorey_needs_the_final_boss(void) {
    DigiviceState s = ready(DigiviceFormFangorey);
    uint32_t at = due(DigiviceFormFangorey);
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormNone);
    s.cleared = 0x7F;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormNone);
    s.cleared = 0xFF;
    CHECK(digivice_evolution_next(&s, at) == DigiviceFormNularca);
}

static void test_end_forms_and_legacy_never_evolve(void) {
    const uint8_t forms[] = {
        DigiviceFormAureola, DigiviceFormNularca, DigiviceFormPrismar, DigiviceFormBaliza,
        DigiviceFormBitling, DigiviceFormTagdrake, DigiviceFormVirubyte,
    };
    for(size_t i = 0; i < sizeof(forms); i++) {
        DigiviceState s = ready(forms[i]);
        s.battles = 200;
        s.wins_stage = 200;
        CHECK(digivice_evolution_next(&s, T0 + 100UL * 24 * 3600) == DigiviceFormNone);
    }
}

int main(void) {
    test_nothing_before_the_timer();
    test_babies_follow_their_line();
    test_korno_branches_in_the_listed_order();
    test_other_rookies();
    test_champions_need_the_battle_record();
    test_mega_needs_few_mistakes_and_a_strong_record();
    test_fangorey_needs_the_final_boss();
    test_end_forms_and_legacy_never_evolve();
    if(failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("evolution: ok\n");
    return 0;
}
