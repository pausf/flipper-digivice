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
    DigiviceStrNoticeOnlyAtNight,
    DigiviceStrNoticeNothingToClean,
    DigiviceStrNoticeHealthy,
    DigiviceStrNoticeFull,

    DigiviceStrFeedHunger,
    DigiviceStrFeedYum,
    DigiviceStrFeedMeat,
    DigiviceStrFeedProtein,
    DigiviceStrFeedVigor,

    DigiviceStrTrainHint,
    DigiviceStrTrainHit,
    DigiviceStrTrainMiss,

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
    DigiviceStrStageEgg,
    DigiviceStrStageBaby,
    DigiviceStrStageBaby2,
    DigiviceStrStageRookie,
    DigiviceStrStageChampion,
    DigiviceStrStageUltimate,
    DigiviceStrStageMega,

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
    DigiviceStrHomeLightsOff,
    DigiviceStrHomeCall,
    DigiviceStrHomeMeterHunger,
    DigiviceStrHomeMeterMood,

    // v3.1: Expedition, map, Hike, midway, gate, encounter and battle.
    DigiviceStrAttrPortador,
    DigiviceStrAttrRuido,
    DigiviceStrAttrCifrado,
    DigiviceStrAttrNulo,
    DigiviceStrArea1,
    DigiviceStrArea2,
    DigiviceStrArea3,
    DigiviceStrArea4,
    DigiviceStrArea5,
    DigiviceStrArea6,
    DigiviceStrArea7,
    DigiviceStrArea8,
    DigiviceStrExpTitle,
    DigiviceStrExpPaces,
    DigiviceStrExpLeft,
    DigiviceStrExpRate,
    DigiviceStrExpAwake,
    DigiviceStrExpNextSpring,
    DigiviceStrExpNextCache,
    DigiviceStrExpNextShortcut,
    DigiviceStrExpNextBoss,
    DigiviceStrExpHintIdle,
    DigiviceStrExpHintFight,
    DigiviceStrExpHintMidway,
    DigiviceStrExpHintBoss,
    DigiviceStrExpHintGate,
    DigiviceStrExpIdleSick,
    DigiviceStrExpIdleAsleep,
    DigiviceStrExpIdleYoung,
    DigiviceStrExpAllClear,
    DigiviceStrMapTitle,
    DigiviceStrMapDone,
    DigiviceStrMapHere,
    DigiviceStrMapLocked,
    DigiviceStrMapStreak,
    DigiviceStrMapRetreatOk,
    DigiviceStrHikeTitle,
    DigiviceStrHikeHint,
    DigiviceStrHikeCapped,
    DigiviceStrHikeGained,
    DigiviceStrMidTitleSpring,
    DigiviceStrMidTitleCache,
    DigiviceStrMidTitleShortcut,
    DigiviceStrMidSpring,
    DigiviceStrMidCacheShield,
    DigiviceStrMidCacheExp,
    DigiviceStrMidShortcutAsk,
    DigiviceStrMidShortcutKeys,
    DigiviceStrMidShortcutWin,
    DigiviceStrMidShortcutLose,
    DigiviceStrMidSkipped,
    DigiviceStrGateNeed,
    DigiviceStrGateScratch,
    DigiviceStrGateRetreat,
    DigiviceStrEncWildPre,
    DigiviceStrEncWildPost,
    DigiviceStrEncBoss,
    DigiviceStrEncZone,
    DigiviceStrEncPow,
    DigiviceStrEncEdge,
    DigiviceStrEncDisedge,
    DigiviceStrEncNeutral,
    DigiviceStrEncRetreatHint,
    DigiviceStrEncAlert,
    DigiviceStrBtlCharge,
    DigiviceStrBtlShotSmall,
    DigiviceStrBtlShotNormal,
    DigiviceStrBtlShotBig,
    DigiviceStrBtlAim,
    DigiviceStrBtlShotHigh,
    DigiviceStrBtlShotLow,
    DigiviceStrBtlHit,
    DigiviceStrBtlBlocked,
    DigiviceStrBtlBroken,
    DigiviceStrBtlDodge,
    DigiviceStrBtlWhiff,
    DigiviceStrBtlFoeTurn,
    DigiviceStrBtlFoeHit,
    DigiviceStrBtlFoeMiss,
    DigiviceStrBtlShieldHolds,
    DigiviceStrBtlRule,
    DigiviceStrBtlVictory,
    DigiviceStrBtlDefeat,
    DigiviceStrBtlDraw,
    DigiviceStrBtlRetreated,
    DigiviceStrBtlNoEscape,
    DigiviceStrBtlItemShield,
    DigiviceStrBtlHurt,
    DigiviceStrBtlStreakLost,
    DigiviceStrBtlAreaClear,
    DigiviceStrNoticeTooYoung,
    DigiviceStrNoticeAreaLocked,
    DigiviceStrNoticeGone,
    DigiviceStrHomeEncounter,
    DigiviceStrHomeBoss,

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
