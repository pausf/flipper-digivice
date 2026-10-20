// Host-side test for the roster table. Run: applications/services/digivice/test/run.sh
#include "../digivice_roster.h"
#include "../digivice_config.h"

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

typedef struct {
    uint8_t form;
    const char* name;
    uint8_t stage;
    uint8_t attr;
    uint32_t stay_s;
} Expect;

static const Expect expected[] = {
    {DigiviceFormByteegg, "BYTEEGG", DigiviceStageEgg, DigiviceAttrNulo, DIGIVICE_HATCH_S},
    {DigiviceFormPipo, "PIPO", DigiviceStageBaby, DigiviceAttrNulo, DIGIVICE_EVO_BABY1_S},
    {DigiviceFormChispi, "CHISPI", DigiviceStageBaby, DigiviceAttrNulo, DIGIVICE_EVO_BABY1_S},
    {DigiviceFormOrejin, "OREJIN", DigiviceStageBaby2, DigiviceAttrNulo, DIGIVICE_EVO_BABY2_S},
    {DigiviceFormColix, "COLIX", DigiviceStageBaby2, DigiviceAttrNulo, DIGIVICE_EVO_BABY2_S},
    {DigiviceFormKorno, "KORNO", DigiviceStageRookie, DigiviceAttrPortador, DIGIVICE_EVO_ROOKIE_S},
    {DigiviceFormChipko, "CHIPKO",
     DigiviceStageRookie, DigiviceAttrCifrado, DIGIVICE_EVO_ROOKIE_S},
    {DigiviceFormEstatik, "ESTATIK",
     DigiviceStageRookie, DigiviceAttrRuido, DIGIVICE_EVO_ROOKIE_S},
    {DigiviceFormMokobit, "MOKOBIT", DigiviceStageRookie, DigiviceAttrNulo, DIGIVICE_EVO_ROOKIE_S},
    {DigiviceFormAriete, "ARIETE",
     DigiviceStageChampion, DigiviceAttrPortador, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormEcolobo, "ECOLOBO",
     DigiviceStageChampion, DigiviceAttrPortador, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormVampixel, "VAMPIXEL",
     DigiviceStageChampion, DigiviceAttrRuido, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormCerrojo, "CERROJO",
     DigiviceStageChampion, DigiviceAttrCifrado, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormBuhoscan, "BUHOSCAN",
     DigiviceStageChampion, DigiviceAttrCifrado, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormFuzzcan, "FUZZCAN",
     DigiviceStageChampion, DigiviceAttrRuido, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormSilbapo, "SILBAPO",
     DigiviceStageChampion, DigiviceAttrRuido, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormChatarro, "CHATARRO",
     DigiviceStageChampion, DigiviceAttrNulo, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormCharco, "CHARCO", DigiviceStageRookie, DigiviceAttrNulo, DIGIVICE_EVO_CHAMP_S},
    {DigiviceFormLanzero, "LANZERO",
     DigiviceStageUltimate, DigiviceAttrPortador, DIGIVICE_EVO_ULTIMATE_S},
    {DigiviceFormWyvolt, "WYVOLT",
     DigiviceStageUltimate, DigiviceAttrRuido, DIGIVICE_EVO_ULTIMATE_S},
    {DigiviceFormFangorey, "FANGOREY",
     DigiviceStageUltimate, DigiviceAttrRuido, DIGIVICE_EVO_ULTIMATE_S},
    {DigiviceFormBovedax, "BOVEDAX",
     DigiviceStageUltimate, DigiviceAttrCifrado, DIGIVICE_EVO_ULTIMATE_S},
    {DigiviceFormKoloso, "KOLOSO",
     DigiviceStageUltimate, DigiviceAttrNulo, DIGIVICE_EVO_ULTIMATE_S},
    {DigiviceFormAureola, "AUREOLA", DigiviceStageMega, DigiviceAttrPortador, 0},
    {DigiviceFormDrakstat, "DRAKSTAT", DigiviceStageMega, DigiviceAttrRuido, 0},
    {DigiviceFormOrakulo, "ORAKULO", DigiviceStageMega, DigiviceAttrCifrado, 0},
    {DigiviceFormNularca, "NULARCA", DigiviceStageMega, DigiviceAttrNulo, 0},
    {DigiviceFormPrismar, "PRISMAR", DigiviceStageMega, DigiviceAttrPortador, 0},
    {DigiviceFormBaliza, "BALIZA", DigiviceStageMega, DigiviceAttrNulo, 0},
};

static void test_table_matches_the_mockup(void) {
    for(size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        const DigiviceRosterEntry* e = digivice_roster(expected[i].form);
        CHECK(e != NULL);
        if(!e) continue;
        CHECK(strcmp(e->name, expected[i].name) == 0);
        CHECK(e->stage == expected[i].stage);
        CHECK(e->attr == expected[i].attr);
        CHECK(e->stay_s == expected[i].stay_s);
    }
}

static void test_every_form_has_a_short_ascii_unique_name(void) {
    for(uint8_t a = 0; a < DigiviceFormCount; a++) {
        const DigiviceRosterEntry* ea = digivice_roster(a);
        CHECK(ea != NULL && ea->name[0] != '\0' && strlen(ea->name) <= 9);
        if(!ea) continue;
        for(uint8_t b = (uint8_t)(a + 1); b < DigiviceFormCount; b++) {
            const DigiviceRosterEntry* eb = digivice_roster(b);
            CHECK(eb != NULL && strcmp(ea->name, eb->name) != 0);
        }
    }
    CHECK(digivice_roster(DigiviceFormCount) == NULL);
    CHECK(digivice_roster(DigiviceFormNone) == NULL);
}

static void test_legacy_forms_keep_their_old_identity(void) {
    CHECK(digivice_roster(DigiviceFormBitling)->stage == DigiviceStageBaby);
    CHECK(digivice_roster(DigiviceFormBytemon)->stage == DigiviceStageRookie);
    CHECK(digivice_roster(DigiviceFormTagdrake)->attr == DigiviceAttrCifrado);
    CHECK(digivice_roster(DigiviceFormWavedrake)->attr == DigiviceAttrPortador);
    CHECK(digivice_roster(DigiviceFormBeamdrake)->attr == DigiviceAttrRuido);
    CHECK(digivice_roster(DigiviceFormPlugdrake)->attr == DigiviceAttrNulo);
    CHECK(digivice_roster(DigiviceFormVirubyte)->attr == DigiviceAttrRuido);
    // Legacy forms never evolve on their own: Task 9 maps them onto the new tree at load.
    CHECK(digivice_roster(DigiviceFormBitling)->stay_s == 0);
}

int main(void) {
    test_table_matches_the_mockup();
    test_every_form_has_a_short_ascii_unique_name();
    test_legacy_forms_keep_their_old_identity();
    if(failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("roster: ok\n");
    return 0;
}
