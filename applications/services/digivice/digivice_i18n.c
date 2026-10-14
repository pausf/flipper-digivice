#include "digivice_i18n.h"

#include <stddef.h>

// canvas_draw_str decodes UTF-8 (u8g2_DrawUTF8) and the 4x6_tf font holds Latin-1, so accents
// are written as UTF-8 escapes. Each macro is its own literal so a following letter such as
// "E" can never be swallowed into the hex escape.
#define A_ACUTE  "\xC3\x81"
#define E_ACUTE  "\xC3\x89"
#define I_ACUTE  "\xC3\x8D"
#define O_ACUTE  "\xC3\x93"
#define U_ACUTE  "\xC3\x9A"
#define N_TILDE  "\xC3\x91"
#define EXCL_INV "\xC2\xA1"
#define QUES_INV "\xC2\xBF"

static const char* const strings_es[DigiviceStrCount] = {
    [DigiviceStrMenuFeed] = "COMER",
    [DigiviceStrMenuTrain] = "ENTRENAR",
    [DigiviceStrMenuBattle] = "BATALLA",
    [DigiviceStrMenuClean] = "LIMPIAR",
    [DigiviceStrMenuLight] = "LUZ",
    [DigiviceStrMenuCure] = "CURAR",
    [DigiviceStrMenuProfile] = "FICHA",
    [DigiviceStrMenuSettings] = "AJUSTES",

    [DigiviceStrNoticeSick] = EXCL_INV "EST" A_ACUTE " ENFERMO!",
    [DigiviceStrNoticeAsleep] = "EST" A_ACUTE " DURMIENDO",
    [DigiviceStrNoticeAwake] = "EST" A_ACUTE " DESPIERTO",
    [DigiviceStrNoticeNothingToClean] = "NO HAY NADA QUE LIMPIAR",
    [DigiviceStrNoticeHealthy] = "EST" A_ACUTE " SANO",
    [DigiviceStrNoticeFull] = EXCL_INV "EST" A_ACUTE " LLENO!",
    [DigiviceStrNoticeNoVirux] = "NING" U_ACUTE "N VIRUX CERCA",

    [DigiviceStrFeedHunger] = "HAMBRE",
    [DigiviceStrFeedYum] = EXCL_INV N_TILDE "AM!",

    [DigiviceStrTrainHint] = EXCL_INV "OK EN LA ZONA!",
    [DigiviceStrTrainHit] = "FUERZA +1  " A_ACUTE "NIMO +1",
    [DigiviceStrTrainMiss] = "FALLASTE...",

    [DigiviceStrBattleAttack] = "ATACAR",
    [DigiviceStrBattleDefend] = "DEFENDER",
    [DigiviceStrBattleFlee] = "HUIR",
    [DigiviceStrBattleAim] = EXCL_INV "PULSA OK A TIEMPO!",
    [DigiviceStrBattleFoeStrikes] = EXCL_INV "EL VIRUX ATACA!",
    [DigiviceStrBattleFled] = EXCL_INV "ESCAPASTE!",
    [DigiviceStrBattleCrit] = EXCL_INV "BYTE CR" I_ACUTE "TICO!",
    [DigiviceStrBattleGlancing] = "GOLPE D" E_ACUTE "BIL",
    [DigiviceStrBattleMiss] = "CORRUPTO... " EXCL_INV "FALLO!",
    [DigiviceStrBattleAppeared] = EXCL_INV "APARECI" O_ACUTE " UN VIRUX!",
    [DigiviceStrBattleWon] = EXCL_INV "VIRUX BORRADO! +EXP",
    [DigiviceStrBattleLost] = "SISTEMA CORRUPTO...",

    [DigiviceStrCleanDone] = EXCL_INV "LIMPIO!",
    [DigiviceStrLightYawn] = "BOSTEZO...",
    [DigiviceStrLightNight] = "BUENAS NOCHES",
    [DigiviceStrLightOn] = EXCL_INV "LUZ ENCENDIDA!",
    [DigiviceStrCureDone] = EXCL_INV "CURADO!",
    [DigiviceStrCureMoreDose] = "1 DOSIS M" A_ACUTE "S",

    [DigiviceStrProfileHunger] = "HAMBRE",
    [DigiviceStrProfileMood] = A_ACUTE "NIMO",
    [DigiviceStrProfileCorruption] = "CORRUP",
    [DigiviceStrProfileStrength] = "FUERZA",
    [DigiviceStrProfileMistakes] = "FALLOS",
    [DigiviceStrModeNormal] = "NORMAL",
    [DigiviceStrModeHacker] = "HACKER",
    [DigiviceStrExp] = "EXP",

    [DigiviceStrSettingsLanguage] = "IDIOMA",
    [DigiviceStrLangName] = "ESPA" N_TILDE "OL",
    [DigiviceStrSettingsMode] = "MODO",
    [DigiviceStrSettingsReset] = "REINICIAR BICHO",
    [DigiviceStrConfirmSure] = QUES_INV "SEGURO?",
    [DigiviceStrConfirmReally] = QUES_INV "DE VERDAD?",
    [DigiviceStrConfirmHint] = "OK=S" I_ACUTE "  ATR" A_ACUTE "S=NO",
    [DigiviceStrResetDone] = EXCL_INV "NUEVO HUEVO!",

    [DigiviceStrErrorService] = "SERVICIO DIGIVICE",
    [DigiviceStrErrorNotRunning] = "NO EST" A_ACUTE " ACTIVO",
    [DigiviceStrErrorBack] = "> ATR" A_ACUTE "S PARA SALIR",

    [DigiviceStrEvolution] = "> " EXCL_INV "EVOLUCI" O_ACUTE "N!",
    [DigiviceStrHomeVirux] = "!VIRUX",
};

static const char* const strings_en[DigiviceStrCount] = {
    [DigiviceStrMenuFeed] = "FEED",
    [DigiviceStrMenuTrain] = "TRAIN",
    [DigiviceStrMenuBattle] = "BATTLE",
    [DigiviceStrMenuClean] = "CLEAN",
    [DigiviceStrMenuLight] = "LIGHT",
    [DigiviceStrMenuCure] = "CURE",
    [DigiviceStrMenuProfile] = "PROFILE",
    [DigiviceStrMenuSettings] = "SETTINGS",

    [DigiviceStrNoticeSick] = "IT'S SICK!",
    [DigiviceStrNoticeAsleep] = "IT'S ASLEEP",
    [DigiviceStrNoticeAwake] = "IT'S AWAKE",
    [DigiviceStrNoticeNothingToClean] = "NOTHING TO CLEAN",
    [DigiviceStrNoticeHealthy] = "IT'S HEALTHY",
    [DigiviceStrNoticeFull] = "IT'S FULL!",
    [DigiviceStrNoticeNoVirux] = "NO VIRUX NEARBY",

    [DigiviceStrFeedHunger] = "HUNGER",
    [DigiviceStrFeedYum] = "YUM!",

    [DigiviceStrTrainHint] = "OK IN THE ZONE!",
    [DigiviceStrTrainHit] = "STR +1  MOOD +1",
    [DigiviceStrTrainMiss] = "MISSED...",

    [DigiviceStrBattleAttack] = "ATTACK",
    [DigiviceStrBattleDefend] = "DEFEND",
    [DigiviceStrBattleFlee] = "FLEE",
    [DigiviceStrBattleAim] = "TIME IT! PRESS OK",
    [DigiviceStrBattleFoeStrikes] = "VIRUX STRIKES!",
    [DigiviceStrBattleFled] = "GOT AWAY SAFELY",
    [DigiviceStrBattleCrit] = "CRITICAL BYTE!",
    [DigiviceStrBattleGlancing] = "GLANCING HIT",
    [DigiviceStrBattleMiss] = "CORRUPTED... MISS!",
    [DigiviceStrBattleAppeared] = "A VIRUX APPEARED!",
    [DigiviceStrBattleWon] = "VIRUX DELETED! EXP UP",
    [DigiviceStrBattleLost] = "SYSTEM CORRUPTED...",

    [DigiviceStrCleanDone] = "CLEAN!",
    [DigiviceStrLightYawn] = "YAWN...",
    [DigiviceStrLightNight] = "GOOD NIGHT",
    [DigiviceStrLightOn] = "LIGHTS ON!",
    [DigiviceStrCureDone] = "CURED!",
    [DigiviceStrCureMoreDose] = "1 MORE DOSE",

    [DigiviceStrProfileHunger] = "HUNGER",
    [DigiviceStrProfileMood] = "MOOD",
    [DigiviceStrProfileCorruption] = "CORRUP",
    [DigiviceStrProfileStrength] = "POWER",
    [DigiviceStrProfileMistakes] = "ERRORS",
    [DigiviceStrModeNormal] = "NORMAL",
    [DigiviceStrModeHacker] = "HACKER",
    [DigiviceStrExp] = "EXP",

    [DigiviceStrSettingsLanguage] = "LANGUAGE",
    [DigiviceStrLangName] = "ENGLISH",
    [DigiviceStrSettingsMode] = "MODE",
    [DigiviceStrSettingsReset] = "RESET PET",
    [DigiviceStrConfirmSure] = "SURE?",
    [DigiviceStrConfirmReally] = "REALLY?",
    [DigiviceStrConfirmHint] = "OK=YES  BACK=NO",
    [DigiviceStrResetDone] = "NEW EGG!",

    [DigiviceStrErrorService] = "DIGIVICE SERVICE",
    [DigiviceStrErrorNotRunning] = "NOT RUNNING",
    [DigiviceStrErrorBack] = "> BACK TO EXIT",

    [DigiviceStrEvolution] = "> EVOLUTION!",
    [DigiviceStrHomeVirux] = "!VIRUX",
};

// Slot widths in pixels, from the layouts in digivice_draw.c and the app screens.
#define SLOT_LABEL   120 // centred label box, full width minus its frame
#define SLOT_TEXTBOX 108 // battle text box, between the ">" and the blinking arrow
#define SLOT_CHOICE  31 // one of the three battle choices
#define SLOT_BAR     25 // profile bar label, left of the 12 bar segments
#define SLOT_SHORT   40 // short tags: header mode, feed hunger box, yum, settings values
#define SLOT_ROW     100 // full-width rows: settings, error screen, train result
#define SLOT_BANNER  88 // evolution banner after its 38 px indent

static const uint8_t budgets[DigiviceStrCount] = {
    [DigiviceStrMenuFeed] = SLOT_LABEL,
    [DigiviceStrMenuTrain] = SLOT_LABEL,
    [DigiviceStrMenuBattle] = SLOT_LABEL,
    [DigiviceStrMenuClean] = SLOT_LABEL,
    [DigiviceStrMenuLight] = SLOT_LABEL,
    [DigiviceStrMenuCure] = SLOT_LABEL,
    [DigiviceStrMenuProfile] = SLOT_LABEL,
    [DigiviceStrMenuSettings] = SLOT_LABEL,

    [DigiviceStrNoticeSick] = SLOT_LABEL,
    [DigiviceStrNoticeAsleep] = SLOT_LABEL,
    [DigiviceStrNoticeAwake] = SLOT_LABEL,
    [DigiviceStrNoticeNothingToClean] = SLOT_LABEL,
    [DigiviceStrNoticeHealthy] = SLOT_LABEL,
    [DigiviceStrNoticeFull] = SLOT_LABEL,
    [DigiviceStrNoticeNoVirux] = SLOT_LABEL,

    [DigiviceStrFeedHunger] = SLOT_SHORT,
    [DigiviceStrFeedYum] = SLOT_SHORT,

    [DigiviceStrTrainHint] = SLOT_ROW,
    [DigiviceStrTrainHit] = SLOT_ROW,
    [DigiviceStrTrainMiss] = SLOT_ROW,

    [DigiviceStrBattleAttack] = SLOT_CHOICE,
    [DigiviceStrBattleDefend] = SLOT_CHOICE,
    [DigiviceStrBattleFlee] = SLOT_CHOICE,
    [DigiviceStrBattleAim] = SLOT_TEXTBOX,
    [DigiviceStrBattleFoeStrikes] = SLOT_TEXTBOX,
    [DigiviceStrBattleFled] = SLOT_TEXTBOX,
    [DigiviceStrBattleCrit] = SLOT_TEXTBOX,
    [DigiviceStrBattleGlancing] = SLOT_TEXTBOX,
    [DigiviceStrBattleMiss] = SLOT_TEXTBOX,
    [DigiviceStrBattleAppeared] = SLOT_TEXTBOX,
    [DigiviceStrBattleWon] = SLOT_TEXTBOX,
    [DigiviceStrBattleLost] = SLOT_TEXTBOX,

    [DigiviceStrCleanDone] = SLOT_LABEL,
    [DigiviceStrLightYawn] = SLOT_LABEL,
    [DigiviceStrLightNight] = SLOT_LABEL,
    [DigiviceStrLightOn] = SLOT_LABEL,
    [DigiviceStrCureDone] = SLOT_LABEL,
    [DigiviceStrCureMoreDose] = SLOT_LABEL,

    [DigiviceStrProfileHunger] = SLOT_BAR,
    [DigiviceStrProfileMood] = SLOT_BAR,
    [DigiviceStrProfileCorruption] = SLOT_BAR,
    [DigiviceStrProfileStrength] = SLOT_BAR,
    // "%s %u" with up to 3 digits must fit the 40 px column under the portrait.
    [DigiviceStrProfileMistakes] = 23,
    [DigiviceStrModeNormal] = SLOT_SHORT,
    [DigiviceStrModeHacker] = SLOT_SHORT,
    [DigiviceStrExp] = 11,

    [DigiviceStrSettingsLanguage] = SLOT_SHORT + 16,
    [DigiviceStrLangName] = SLOT_SHORT,
    [DigiviceStrSettingsMode] = SLOT_SHORT + 16,
    [DigiviceStrSettingsReset] = SLOT_ROW,
    [DigiviceStrConfirmSure] = SLOT_LABEL,
    [DigiviceStrConfirmReally] = SLOT_LABEL,
    [DigiviceStrConfirmHint] = SLOT_LABEL,
    [DigiviceStrResetDone] = SLOT_LABEL,

    [DigiviceStrErrorService] = SLOT_ROW,
    [DigiviceStrErrorNotRunning] = SLOT_ROW,
    [DigiviceStrErrorBack] = SLOT_ROW,

    [DigiviceStrEvolution] = SLOT_BANNER,
    // "%s %lum" with a 2-digit minute count must fit the 46 px alert box.
    [DigiviceStrHomeVirux] = 23,
};

const char* digivice_str(DigiviceLang lang, DigiviceStr id) {
    if(id >= DigiviceStrCount) return "";
    const char* const* table = lang == DigiviceLangEn ? strings_en : strings_es;
    return table[id] ? table[id] : "";
}

uint16_t digivice_i18n_text_width(const char* text) {
    uint16_t glyphs = 0;
    for(const unsigned char* p = (const unsigned char*)text; *p; p++) {
        // UTF-8 continuation bytes are 10xxxxxx and add no glyph.
        if((*p & 0xC0) != 0x80) glyphs++;
    }
    return glyphs ? (uint16_t)(glyphs * 4 - 1) : 0;
}

uint16_t digivice_i18n_budget(DigiviceStr id) {
    return id < DigiviceStrCount ? budgets[id] : 0;
}
