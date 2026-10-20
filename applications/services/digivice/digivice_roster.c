#include "digivice_roster.h"
#include "digivice_config.h"

#define E(n, st, at, t) {.name = n, .stage = st, .attr = at, .stay_s = t}

static const DigiviceRosterEntry roster[DigiviceFormCount] = {
    [DigiviceFormByteegg] = E("BYTEEGG", DigiviceStageEgg, DigiviceAttrNulo, DIGIVICE_HATCH_S),
    // Legacy v1 to v3 forms: they never evolve and only exist for old saves.
    [DigiviceFormBitling] = E("BITLING", DigiviceStageBaby, DigiviceAttrNulo, 0),
    [DigiviceFormBytemon] = E("BYTEMON", DigiviceStageRookie, DigiviceAttrNulo, 0),
    [DigiviceFormTagdrake] = E("TAGDRAKE", DigiviceStageChampion, DigiviceAttrCifrado, 0),
    [DigiviceFormWavedrake] = E("WAVEDRAKE", DigiviceStageChampion, DigiviceAttrPortador, 0),
    [DigiviceFormBeamdrake] = E("BEAMDRAKE", DigiviceStageChampion, DigiviceAttrRuido, 0),
    [DigiviceFormPlugdrake] = E("PLUGDRAKE", DigiviceStageChampion, DigiviceAttrNulo, 0),
    [DigiviceFormVirubyte] = E("VIRUBYTE", DigiviceStageChampion, DigiviceAttrRuido, 0),

    [DigiviceFormPipo] = E("PIPO", DigiviceStageBaby, DigiviceAttrNulo, DIGIVICE_EVO_BABY1_S),
    [DigiviceFormChispi] = E("CHISPI", DigiviceStageBaby, DigiviceAttrNulo, DIGIVICE_EVO_BABY1_S),
    [DigiviceFormOrejin] = E("OREJIN", DigiviceStageBaby2, DigiviceAttrNulo, DIGIVICE_EVO_BABY2_S),
    [DigiviceFormColix] = E("COLIX", DigiviceStageBaby2, DigiviceAttrNulo, DIGIVICE_EVO_BABY2_S),
    [DigiviceFormKorno] = E("KORNO",
        DigiviceStageRookie, DigiviceAttrPortador, DIGIVICE_EVO_ROOKIE_S),
    [DigiviceFormChipko] = E("CHIPKO",
        DigiviceStageRookie, DigiviceAttrCifrado, DIGIVICE_EVO_ROOKIE_S),
    [DigiviceFormEstatik] = E("ESTATIK",
        DigiviceStageRookie, DigiviceAttrRuido, DIGIVICE_EVO_ROOKIE_S),
    [DigiviceFormMokobit] = E("MOKOBIT",
        DigiviceStageRookie, DigiviceAttrNulo, DIGIVICE_EVO_ROOKIE_S),
    [DigiviceFormAriete] = E("ARIETE",
        DigiviceStageChampion, DigiviceAttrPortador, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormEcolobo] = E("ECOLOBO",
        DigiviceStageChampion, DigiviceAttrPortador, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormVampixel] = E("VAMPIXEL",
        DigiviceStageChampion, DigiviceAttrRuido, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormCerrojo] = E("CERROJO",
        DigiviceStageChampion, DigiviceAttrCifrado, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormBuhoscan] = E("BUHOSCAN",
        DigiviceStageChampion, DigiviceAttrCifrado, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormFuzzcan] = E("FUZZCAN",
        DigiviceStageChampion, DigiviceAttrRuido, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormSilbapo] = E("SILBAPO",
        DigiviceStageChampion, DigiviceAttrRuido, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormChatarro] = E("CHATARRO",
        DigiviceStageChampion, DigiviceAttrNulo, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormCharco] = E("CHARCO",
        DigiviceStageRookie, DigiviceAttrNulo, DIGIVICE_EVO_CHAMP_S),
    [DigiviceFormLanzero] = E("LANZERO",
        DigiviceStageUltimate, DigiviceAttrPortador, DIGIVICE_EVO_ULTIMATE_S),
    [DigiviceFormWyvolt] = E("WYVOLT",
        DigiviceStageUltimate, DigiviceAttrRuido, DIGIVICE_EVO_ULTIMATE_S),
    [DigiviceFormFangorey] = E("FANGOREY",
        DigiviceStageUltimate, DigiviceAttrRuido, DIGIVICE_EVO_ULTIMATE_S),
    [DigiviceFormBovedax] = E("BOVEDAX",
        DigiviceStageUltimate, DigiviceAttrCifrado, DIGIVICE_EVO_ULTIMATE_S),
    [DigiviceFormKoloso] = E("KOLOSO",
        DigiviceStageUltimate, DigiviceAttrNulo, DIGIVICE_EVO_ULTIMATE_S),
    [DigiviceFormAureola] = E("AUREOLA", DigiviceStageMega, DigiviceAttrPortador, 0),
    [DigiviceFormDrakstat] = E("DRAKSTAT", DigiviceStageMega, DigiviceAttrRuido, 0),
    [DigiviceFormOrakulo] = E("ORAKULO", DigiviceStageMega, DigiviceAttrCifrado, 0),
    [DigiviceFormNularca] = E("NULARCA", DigiviceStageMega, DigiviceAttrNulo, 0),
    [DigiviceFormPrismar] = E("PRISMAR", DigiviceStageMega, DigiviceAttrPortador, 0),
    [DigiviceFormBaliza] = E("BALIZA", DigiviceStageMega, DigiviceAttrNulo, 0),
};

const DigiviceRosterEntry* digivice_roster(uint8_t form) {
    return form < DigiviceFormCount ? &roster[form] : NULL;
}
