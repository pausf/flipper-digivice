#include "digivice_evolution.h"
#include "digivice_config.h"
#include "digivice_roster.h"

static bool record_ok(const DigiviceState* s, uint8_t min_battles, uint8_t pct) {
    return s->battles >= min_battles &&
           (uint32_t)s->wins_stage * 100U >= (uint32_t)s->battles * pct;
}

static uint8_t baby_next(const DigiviceState* s) {
    bool good = s->evo_mistakes <= DIGIVICE_EVO_MISTAKES_GOOD;
    switch(s->form) {
    case DigiviceFormPipo:
        return DigiviceFormOrejin;
    case DigiviceFormChispi:
        return DigiviceFormColix;
    case DigiviceFormOrejin:
        return good ? DigiviceFormKorno : DigiviceFormEstatik;
    case DigiviceFormColix:
        return good ? DigiviceFormChipko : DigiviceFormMokobit;
    default:
        return DigiviceFormNone;
    }
}

// Conditions are checked in the order of the tree; a Rookie that matches none becomes CHARCO.
static uint8_t rookie_next(const DigiviceState* s) {
    uint8_t t = s->train_count;
    switch(s->form) {
    case DigiviceFormKorno:
        if(t >= DIGIVICE_EVO_TRAIN_HI && s->evo_mistakes <= 1) return DigiviceFormAriete;
        if(t >= DIGIVICE_EVO_TRAIN_MID && t < DIGIVICE_EVO_TRAIN_HI) return DigiviceFormEcolobo;
        if(s->evo_mistakes >= 2) return DigiviceFormVampixel;
        break;
    case DigiviceFormEstatik:
        if(t >= DIGIVICE_EVO_TRAIN_LO) return DigiviceFormFuzzcan;
        if(s->overfeed >= DIGIVICE_EVO_OVERFEED) return DigiviceFormSilbapo;
        break;
    case DigiviceFormChipko:
        if(t >= DIGIVICE_EVO_TRAIN_HI) return DigiviceFormCerrojo;
        if(s->night_owl >= DIGIVICE_EVO_NIGHT_OWL) return DigiviceFormBuhoscan;
        break;
    case DigiviceFormMokobit:
        if(t >= DIGIVICE_EVO_TRAIN_LO) return DigiviceFormChatarro;
        break;
    default:
        return DigiviceFormNone;
    }
    return DigiviceFormCharco;
}

static uint8_t champion_next(const DigiviceState* s) {
    if(!record_ok(s, DIGIVICE_EVO_ULT_BATTLES, DIGIVICE_EVO_ULT_PCT)) return DigiviceFormNone;
    switch(s->form) {
    case DigiviceFormAriete:
    case DigiviceFormEcolobo:
        return DigiviceFormLanzero;
    case DigiviceFormVampixel:
    case DigiviceFormFuzzcan:
        return DigiviceFormWyvolt;
    case DigiviceFormSilbapo:
    case DigiviceFormCharco: // the redemption path
        return DigiviceFormFangorey;
    case DigiviceFormCerrojo:
    case DigiviceFormBuhoscan:
        return DigiviceFormBovedax;
    case DigiviceFormChatarro:
        return DigiviceFormKoloso;
    default:
        return DigiviceFormNone;
    }
}

static uint8_t ultimate_next(const DigiviceState* s) {
    if(s->form == DigiviceFormFangorey) {
        // The final boss (area 8) must have fallen.
        return (s->cleared & 0x80) ? DigiviceFormNularca : DigiviceFormNone;
    }
    bool ok = s->evo_mistakes <= DIGIVICE_EVO_MEGA_MISTAKES &&
              record_ok(s, DIGIVICE_EVO_MEGA_BATTLES, DIGIVICE_EVO_MEGA_PCT);
    if(!ok) return DigiviceFormNone;
    switch(s->form) {
    case DigiviceFormLanzero:
        return DigiviceFormAureola;
    case DigiviceFormWyvolt:
        return DigiviceFormDrakstat;
    case DigiviceFormBovedax:
        return DigiviceFormOrakulo;
    case DigiviceFormKoloso:
        return DigiviceFormNularca;
    default:
        return DigiviceFormNone;
    }
}

uint8_t digivice_evolution_next(const DigiviceState* s, uint32_t now) {
    const DigiviceRosterEntry* e = digivice_roster(s->form);
    if(!e || e->stay_s == 0) return DigiviceFormNone;
    if(now < s->stage_since || now - s->stage_since < e->stay_s) return DigiviceFormNone;
    // CHARCO has the Rookie stage but waits a Champion's time and follows the Champion rule.
    if(s->form == DigiviceFormCharco) return champion_next(s);
    switch(e->stage) {
    case DigiviceStageBaby:
    case DigiviceStageBaby2:
        return baby_next(s);
    case DigiviceStageRookie:
        return rookie_next(s);
    case DigiviceStageChampion:
        return champion_next(s);
    case DigiviceStageUltimate:
        return ultimate_next(s);
    default:
        return DigiviceFormNone;
    }
}
