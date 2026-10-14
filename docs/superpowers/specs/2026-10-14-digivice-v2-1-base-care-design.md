# Digivice v2.1: base and care — Design

Date: 2026-10-14
Status: Approved in brainstorming, pending written-spec review
Builds on: `docs/superpowers/specs/2026-10-13-digivice-design.md` (v1)

## Context

v1 runs on the device. User feedback after the first try:

- Nothing explains itself on screen. "Train" seemed to do nothing, and battles never appeared.
- The UI is English-only. Spanish should be the default.
- The game should work like a store-bought V-Pet/Digivice, without needing constant
  NFC/Sub-GHz use (Normal mode). The scan-driven play stays available (Hacker mode).
- The user wants richer animations and classic V-Pet care.

v2 ships in three deliveries, each with its own spec, plan, CI build and device test:

| Delivery | Scope |
|---|---|
| **v2.1 (this spec)** | Spanish/English, Settings (language and mode), icon menu, poop/light/medicine/sickness, 5-minute hatch, new Feed and Profile screens |
| v2.2 | New battle (4 named moves per form, timing-bar crit, attack animations), 4 VIRUX × 3 sizes + rare boss, time-based encounters, auto-start encounter, bestiary |
| v2.3 | 4 training minigames (Strike, Speed, Reflex, Endurance) and evolution driven by training |

The approved mockups are in `.superpowers/brainstorm/12173-1791968437/content/`
(`v2-designs.html` and `v2-classic.html`, style "HUD cyber", 128×64 1-bit). v2.1
implements the icon menu, Clean, Light, Cure, Feed and Profile screens from them.

## Constraints

- All v1 constraints still apply. No new stock-firmware edits are needed: only
  digivice files change, plus the desktop home view.
- Every tuning number lives in `applications/services/digivice/digivice_config.h`.
- The rules module stays pure C (no furi) and is test-driven on the host.
- The existing pet must survive the update (see the v1→v2 migration below).

## Languages

- New `applications/services/digivice/digivice_i18n.h/.c`:
  - `typedef enum { DigiviceLangEs, DigiviceLangEn, DigiviceLangCount } DigiviceLang;`
  - `typedef enum { DigiviceStr..., DigiviceStrCount } DigiviceStr;` covers every
    user-visible string in the app and the home view.
  - `const char* digivice_str(DigiviceLang lang, DigiviceStr id);` backed by two
    `static const char* const` tables of size `DigiviceStrCount`.
- Spanish is the default for a new pet and for a migrated v1 pet.
- **Accents.** The HUD text font changes from `u8g2_font_4x6_tr` (ASCII) to
  `u8g2_font_4x6_tf` (Latin-1, `lib/u8g2/u8g2.h:3897`, 1477 bytes).
  `canvas_draw_str` decodes UTF-8 (`u8g2_DrawUTF8`, `gui/canvas.c:183`), so Spanish
  strings are written as UTF-8 escapes in the C source: `"\xC2\xA1"` for ¡,
  `"\xC3\xB1"` for ñ, `"\xC3\xA1"` for á, and so on. The source file stays ASCII-only.
- Form and VIRUX names are proper nouns and are not translated.
- Pure-C helper `digivice_i18n_text_width(const char*)` (4 px per glyph minus 1).
  Every string must fit the width budget of its slot.

## Persisted state (v2) and migration

`DigiviceState` gains these fields. All are fixed-width, appended after the existing
ones, with the struct kept naturally aligned:

| Field | Type | Meaning |
|---|---|---|
| `lang` | `uint8_t` | `DigiviceLang` |
| `mode` | `uint8_t` | `DigiviceModeNormal` (default) or `DigiviceModeHacker` |
| `poop` | `uint8_t` | 0..`DIGIVICE_POOP_MAX` |
| `sick` | `uint8_t` | 0/1 |
| `care_mistakes` | `uint8_t` | since the last cure, capped at 255 |
| `light_on` | `uint8_t` | 0/1 |
| `doses_needed` | `uint8_t` | remaining doses while sick |
| `reserved2` | `uint8_t` | padding |
| `poop_acc_s` | `uint32_t` | awake seconds toward the next poop |
| `poop_dirty_since` | `uint32_t` | timestamp the oldest uncleaned poop appeared, 0 when none |
| `starving_since` | `uint32_t` | timestamp hunger hit 0, 0 otherwise |
| `sick_since` | `uint32_t` | timestamp sickness started, 0 otherwise |
| `light_on_asleep_s` | `uint32_t` | seconds the light stayed on during the current sleep window |

- The saved_struct version goes from `0x01` to `0x02`. The magic stays the same.
- **Migration.** On load, the service calls `saved_struct_get_metadata`
  (`lib/toolbox/saved_struct.h:56`).
  - Version `0x02`: load normally.
  - Version `0x01`: load into a `DigiviceStateV1` struct (an exact copy of the v1
    layout, kept in `digivice_rules.h`), then call the pure function
    `digivice_rules_migrate_v1(const DigiviceStateV1*, DigiviceState*)`. It copies
    every field and sets the new ones to: `lang = Es`, `mode = Normal`,
    `light_on = 1`, everything else 0. Then the state is marked dirty so it is
    saved as v2.
  - Any other version, or any load failure: a fresh egg, as in v1.

## Rules (pure C, `digivice_rules.c`)

### Start

- `DIGIVICE_HATCH_S` goes from 3600 to **300** (5 minutes). Baby to Rookie stays
  at exp ≥ 50.

### Modes

- **Normal:** `digivice_rules_on_deed` returns without changing state (no EXP, no
  data, no VIRUX).
- **Hacker:** v1 deed behaviour (EXP, data, VIRUX spawn).
- Both modes: training gives EXP (`DIGIVICE_TRAIN_EXP_OK` = 3,
  `DIGIVICE_TRAIN_EXP_FAIL` = 1) on top of the v1 strength/mood effects.
- `digivice_rules_set_mode(s, mode)` and `digivice_rules_set_lang(s, lang)` validate
  their range.

### Poop

- One poop every `DIGIVICE_POOP_EVERY_S` = 3 h of awake time, capped at
  `DIGIVICE_POOP_MAX` = 4. Only Baby and later stages poop; an Egg never does.
- With poop ≥ 3, mood decays at twice the normal rate.
- If any poop stays uncleaned for more than `DIGIVICE_POOP_MISTAKE_S` = 6 h, add one
  care mistake and restart `poop_dirty_since` at the current time. This happens at
  most once per 6 h window.
- `digivice_rules_clean(s, now)`: if poop > 0, set it to 0, clear `poop_dirty_since`,
  give mood +1 (capped) and return true. Otherwise return false.

### Sickness and medicine

- The pet gets sick when either holds:
  - `care_mistakes ≥ DIGIVICE_SICK_MISTAKES` (2);
  - hunger has been 0 for ≥ `DIGIVICE_STARVE_SICK_S` (24 h of wall time, tracked by
    `starving_since`).
- While sick:
  - mood decays at twice the normal rate;
  - corruption rises at +5/h even if hunger and mood are above 0;
  - training and battling are refused. The rules return a refusal reason, and the UI
    shows a localized notice.
- On becoming sick: `doses_needed = 1`. When `now - sick_since ≥ 12 h`, it goes up
  to 2.
- `digivice_rules_cure(s, now)`: if sick, `doses_needed -= 1`. When it reaches 0,
  `sick = 0`, `care_mistakes = 0` and `sick_since = 0`. Returns whether a dose was
  used. A cure on a healthy pet is refused.

### Light

- During the sleep window (23:00–07:00, the v1 helper) the pet sleeps.
- If `light_on` during sleep, `light_on_asleep_s` accumulates. Crossing
  `DIGIVICE_LIGHT_MISTAKE_S` = 1 h adds one care mistake and sets mood −1, once per
  sleep window.
- `light_on` and `light_on_asleep_s` reset when the window ends: the light
  switches back on at 07:00.
- `digivice_rules_toggle_light(s, now)`: while asleep, it toggles. While awake,
  turning it off is refused (the light stays on) and the function returns a refusal.

### Care mistakes

- Counted in `care_mistakes`, shown on the Profile screen, cleared by a full cure.
- v2.3 will use them as a soft evolution modifier. v2.1 does not.

All of these rules respect the v1 elapsed-time clamp [0, 48 h] and step event by
event, as v1 does.

## Service changes (`digivice_srv.c`, `digivice.h`)

- New public calls, all non-blocking from a pubsub callback's point of view:
  - `digivice_clean`, `digivice_cure` and `digivice_toggle_light` return a result
    enum (OK, or a refusal reason);
  - `digivice_set_lang` and `digivice_set_mode`;
  - `digivice_reset` (fresh egg; keeps lang and mode).
- `digivice_train` gains the EXP rule above and returns the refusal reason when sick
  or asleep.
- The load path implements the v1→v2 migration.
- Notifications:
  - becoming sick: red LED blink + short vibration;
  - poop appearing: single soft blink.

  Both are silent in the sleep window, as the v1 evolution buzzer is.

## App (`applications/main/digivice/`)

- **Main screen** (replaces the v1 text menu):
  - top bar icons: COMER, ENTRENAR, BATALLA, LIMPIAR;
  - bottom bar icons: LUZ, CURAR, FICHA, AJUSTES;
  - 9×9 1-bit icons, in the draw kit, taken from the mockup bitmaps;
  - the pet walks in the middle band and shows poop (up to 4 piles), the skull when
    sick, sleep (closed eyes, Z's, dark dither) when the light is off at night.
- **Controls:** ←/→ move across the 8 icons (wrapping top row then bottom row), OK
  opens one, Back exits the app. ↑/↓ jump between the top and bottom rows.
- **Refusals:** a blocked action shows a localized notice ("¡ESTÁ ENFERMO!",
  "ESTÁ DURMIENDO", "NO HAY NADA QUE LIMPIAR") for about 1.5 s. The app never does
  nothing silently.
- **COMER:** a data chip drops in, the pet takes three bites with crumbs, then hearts
  and the hunger hearts fill. A full pet shows "¡ESTÁ LLENO!".
- **ENTRENAR:** the v1 timing minigame, plus an on-screen instruction line
  ("¡OK EN LA ZONA!") and a result line showing the EXP gained. The 4 new minigames
  arrive in v2.3.
- **BATALLA:** the v1 battle with translated strings. The v2.2 redesign replaces it.
- **LIMPIAR:** a wave sweeps right to left and removes the poop, then "¡LIMPIO!" and
  hearts.
- **LUZ:** while asleep, the screen dithers to dark with moon, stars and Z's
  ("BUENAS NOCHES"). Turning it on again reverses this. While awake, the refusal
  notice shows.
- **CURAR:** the syringe slides in and pushes, sparkles appear, and the skull
  disappears with "¡CURADO!". If a dose is still needed: "1 DOSIS MÁS".
- **FICHA:** the animated profile card from the mockup. It shows the portrait with a
  rotating ring; bars for hambre, ánimo, corrupción and fuerza; an EXP bar; data per
  type (Hacker mode only); care mistakes; and the current mode.
- **AJUSTES:** a list with Idioma (Español / English), Modo (Normal / Hacker), and
  Reiniciar bicho. The reset asks for confirmation twice: "¿SEGURO?", then
  "¿DE VERDAD?".

## Desktop home view

- Draws poop (up to 4), the sickness skull, and sleep with the light off (dark dither
  and Z's), using the same draw-kit functions as the app.
- Strings are localized through `digivice_str` with the state's `lang`.

## Error handling

- State migration as described above. Failures fall back to a fresh egg.
- An out-of-range `lang` or `mode` in a loaded file is clamped to the default
  (Es / Normal).
- Every refusal is visible and localized (see App).

## Testing

- **Host rules tests (TDD).** These areas are new or changed:
  - hatch at 300 s;
  - Normal mode ignores deeds, Hacker mode keeps v1 behaviour;
  - training EXP;
  - poop cadence, cap, Egg exemption, the double mood decay at poop ≥ 3, and the
    6 h mistake applied once per window;
  - clean;
  - sickness from mistakes and from starvation, refusals while sick, faster decay,
    corruption while sick;
  - doses, 1 versus 2 after 12 h;
  - cure resets the mistakes;
  - light: mistake after 1 h on during sleep, once per window, reset at 07:00, and
    the refusal while awake;
  - `digivice_rules_migrate_v1` field-by-field;
  - out-of-range lang and mode clamping.
- **Host i18n test.** It covers three things:
  - both tables have every `DigiviceStr`, none NULL;
  - every string fits its slot width budget (the budget table lives next to the
    strings);
  - the source contains only ASCII bytes, so Latin-1 characters are always escaped.
- **CI.** The fbt build stays green.
- **Device check.** The user verifies:
  - the existing pet survives the update, with the same form and EXP;
  - Spanish by default and the language switch;
  - each icon screen and its animation;
  - poop and the skull on the home screen;
  - the stock Flipper still works.

## Out of scope for v2.1

- New battle system, VIRUX roster, encounters, bestiary (v2.2).
- New training minigames and training-driven evolution (v2.3).
- Translating the stock Flipper firmware.
