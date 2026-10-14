#pragma once

// Pure C: no furi includes, so the host test can compile this file with plain cc.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DigiviceLangEs,
    DigiviceLangEn,
    DigiviceLangCount,
} DigiviceLang;

typedef enum {
    // Main screen icons, in screen order.
    DigiviceStrMenuFeed,
    DigiviceStrMenuTrain,
    DigiviceStrMenuBattle,
    DigiviceStrMenuClean,
    DigiviceStrMenuLight,
    DigiviceStrMenuCure,
    DigiviceStrMenuProfile,
    DigiviceStrMenuSettings,

    // Refusal notices.
    DigiviceStrNoticeSick,
    DigiviceStrNoticeAsleep,
    DigiviceStrNoticeAwake,
    DigiviceStrNoticeNothingToClean,
    DigiviceStrNoticeHealthy,
    DigiviceStrNoticeFull,
    DigiviceStrNoticeNoVirux,

    DigiviceStrFeedHunger,
    DigiviceStrFeedYum,

    DigiviceStrTrainHint,
    DigiviceStrTrainHit,
    DigiviceStrTrainMiss,

    DigiviceStrBattleAttack,
    DigiviceStrBattleDefend,
    DigiviceStrBattleFlee,
    DigiviceStrBattleAim,
    DigiviceStrBattleFoeStrikes,
    DigiviceStrBattleFled,
    DigiviceStrBattleCrit,
    DigiviceStrBattleGlancing,
    DigiviceStrBattleMiss,
    DigiviceStrBattleAppeared,
    DigiviceStrBattleWon,
    DigiviceStrBattleLost,

    DigiviceStrCleanDone,
    DigiviceStrLightYawn,
    DigiviceStrLightNight,
    DigiviceStrLightOn,
    DigiviceStrCureDone,
    DigiviceStrCureMoreDose,

    DigiviceStrProfileHunger,
    DigiviceStrProfileMood,
    DigiviceStrProfileCorruption,
    DigiviceStrProfileStrength,
    DigiviceStrProfileMistakes,
    DigiviceStrModeNormal,
    DigiviceStrModeHacker,
    DigiviceStrExp,

    DigiviceStrSettingsLanguage,
    DigiviceStrLangName,
    DigiviceStrSettingsMode,
    DigiviceStrSettingsReset,
    DigiviceStrConfirmSure,
    DigiviceStrConfirmReally,
    DigiviceStrConfirmHint,
    DigiviceStrResetDone,

    DigiviceStrErrorService,
    DigiviceStrErrorNotRunning,
    DigiviceStrErrorBack,

    DigiviceStrEvolution,
    DigiviceStrHomeVirux,

    DigiviceStrCount,
} DigiviceStr;

/** Never NULL: out-of-range arguments fall back to Spanish and to an empty string. */
const char* digivice_str(DigiviceLang lang, DigiviceStr id);

/** Pixel width in the 4x6 HUD font: 4 px per glyph minus 1. UTF-8 sequences count as one glyph. */
uint16_t digivice_i18n_text_width(const char* text);

/** Widest pixel width the string's slot on screen can hold. */
uint16_t digivice_i18n_budget(DigiviceStr id);

#ifdef __cplusplus
}
#endif
