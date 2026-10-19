# Digivice v3.1: battle and Expedition — Design

Date: 2026-10-19
Status: Draft, pending written-spec review
Builds on: `docs/superpowers/specs/2026-10-14-digivice-v2-1-base-care-design.md` (v2.1, branch `digivice-v2.1`)
Inputs: `docs/superpowers/v3-status.md`, `docs/superpowers/research/2026-10-16-vpet-digivice-research.md` (sections 4.1, 4.3, 5, 9.2, 9.5, 9.6, 9.7), mockups `docs/superpowers/mockups/v3/v3-combate.html` and `v3-expedicion.html`.

## Context

v2.1 runs on the device. Two things are missing: the BATALLA icon is a dead end in Normal mode ("SOLO EN MODO HACKER"), and the v1 battle (HP, attack/defend/flee menu) is a stand-in. v3 ships in three deliveries:

| Delivery | Scope |
|---|---|
| **v3.1 (this spec)** | New battle (triangle, charge, aim, 3 hits), Expedition (8 areas, paces, Hike, encounters, midway, bosses), Hacker scan paces, v3 save format, ES/EN strings |
| v3.2 | 28-monster roster, hidden evolution counters, calls and care mistakes, reworked animations |
| v3.3 | Cities, easter eggs with capture, NIDO and companions, scenery polish |

This delivery works with the **current 7 pet forms** only (Byteegg, Bitling, Bytemon, Tagdrake, Wavedrake, Beamdrake, Plugdrake, plus the Virubyte failure form). Champion is the stage ceiling until v3.2, so areas 6 to 8 are intentionally very hard for now.

## Constraints

- All v1 and v2.1 constraints still apply. No stock-firmware edits beyond what v1 and v2 already touched: digivice files, `desktop_view_digivice.c`, `applications/services/digivice/test/run.sh` and `.github/workflows/digivice.yml`. Stock-file diff beyond that: **zero**.
- No Bandai names, art or sprites anywhere. Every monster, boss, area and attribute is original (names come from the approved mockups).
- Every tuning number lives in `applications/services/digivice/digivice_config.h`. The v1-only constants (`DIGIVICE_BASE_DAMAGE`, `DIGIVICE_PLAYER_HP`, `DIGIVICE_VIRUX_HP`, `DIGIVICE_VIRUX_ATK`, `DIGIVICE_TYPE_ADV_X10`, `DIGIVICE_TIMING_HIT_X10`, `DIGIVICE_TIMING_MISS_X10`, `DIGIVICE_FLEE_PCT`) are deleted.
- The rules stay pure C (no furi) and are test-driven on the host. New pure modules: `digivice_battle.h/.c` and `digivice_expedition.h/.c`, compiled by `test/run.sh` next to `digivice_rules.c`.
- **Flash.** About 240 KB was free before v2.1. Target ceiling for this delivery: **at most 40 KB added** (expected about 33 KB, see Budgets). The plan must record the real number from the CI build and stop to re-scope above 40 KB.
- **RAM.** The service stack is 2 KB. No large locals anywhere in the service: `DigiviceState` grows from 84 to 116 bytes and is copied up to three times on the tick path, so the plan re-measures free stack. App scene caches go on the heap, never on the stack.
- Source files stay ASCII-only: accents are `\x` escapes in `digivice_i18n.c`.

## Decisions

The brief left these open. They are resolved here and can be overturned in review.

| # | Decision | Why |
|---|---|---|
| 1 | The BATALLA icon stays in the same slot, label **RUTA / TRAIL**, and opens the Expedition screen in both modes. | Fixes the dead end without moving icons. A short label fits the existing menu slot. |
| 2 | Expedition has no start or stop. There is always one active area. Paces accrue only while the pet is **awake, not sick and at least Baby**. | A V-Pet does not need a "go walking" chore. The user chose a fast start where the Baby already fights weak trail foes (v2 brainstorming); only the Egg cannot. |
| 3 | Area lengths are 480, 600, 720, 840, 960, 1080, 1200, 1440 paces, not the research's 120 to 600. | See "Area table": an awake day is 16 h x 60 = 960 paces, so the average area takes about one awake day. |
| 4 | `paces_since_fight` resets when an encounter **spawns**, not when it is fought. | Otherwise an ignored encounter would re-trigger the guarantee on the very next pace. |
| 5 | Trail encounters and VIRUX encounters share one rule: only one can be pending. VIRUX keeps its existing `virux_type` and `virux_expires` fields. | No duplicate slot, and the v1/v2 fields keep working. |
| 6 | Midway events: **rest spring, item cache, shortcut gamble** (ally rescue is deferred to the NIDO delivery). One fixed kind per area, never expires, never blocks paces. | Ally rescue needs companions, which are out of scope. |
| 7 | The boss gate **refuses** the fight below Champion. There is no "chip damage" fight. | Simpler to test and to explain. The gate screen offers RETREAT or TRAIN. |
| 8 | Foe turns are automatic. The player's agency is charge plus aim. The foe guard flickers and is sampled at the moment the shot lands. | Avoids a coin-flip aim without adding a defence minigame. |
| 9 | A matched guard does not block the shot outright: it **halves the hit chance** (minimum 5%). | Keeps the "3 hits" counter honest and one rule for every case. |
| 10 | No HP anywhere in battle. The state is hits (0 to 3 per side) and a round counter. After round 10 the side with more hits wins, a tie is a **draw** (foe flees, no penalty). | Prevents endless fights at low hit chances. |
| 11 | Hit chance is `100*P/(P+Q)` plus or minus 5 for attribute, clamped to 10..90. The clamp comes from research 9.2. | Research 4.1 has no clamp, 9.2 does. The clamp avoids certain hits and certain misses. |
| 12 | The only item is the **Shield**: it cancels the first foe hit of the next battle. | The mockup's victory screen drops a shield. One item keeps scope small. |
| 13 | Defeat gives an **injury** for 1 h: power x3/4 and a bandage on the HUD. A rest spring clears it. No new care icon. | The mockup shows "HERIDO". Care rework is v3.2. |
| 14 | Pace bookkeeping (`distance`, `paces_since_fight`, `pace_acc_s`) is excluded from "visible" state, and the RNG seed is part of the state. | Paces change every minute. Counting them as visible would write the internal flash every minute. After a reboot, the offline simulation recomputes them from `last_tick`, as it already does for hunger. |
| 15 | `digivice_rules_migrate_v1` now outputs a new `DigiviceStateV2` struct, and a new `digivice_rules_migrate_v2` goes V2 to v3. | A real v1 to v2 to v3 chain. The v1 tests change their output type only. |
| 16 | Hike taps are sent to the service in one batch when the session ends or Back is pressed. | One message and one flash flag per session, not one per tap. |
| 17 | Switching area (from the map) resets the area's progress. It is allowed at any time. | No hidden per-area progress to persist. Areas 1 to 4 already allow retreat, and 5 to 8 treat leaving as losing the streak. |
| 18 | Trail monsters reuse the mockup bestiary names (ESTATIK, CHIPKO, KORNO, MOKOBIT, FUZZCAN, VAMPIXEL, SILBAPO, CHATARRO) with simplified draw-kit versions. v3.2 replaces the art with the full roster sprites. | The mockup already approved them as original designs. |
| 19 | Monster, foe and boss names are proper nouns and are not translated. Attribute and area names are translated. | Same rule as v2.1 for form names. |

## Attributes and the 7 forms

Triangle: **Portador > Ruido > Cifrado > Portador**. **Nulo** is neutral against everyone. English names: Carrier, Noise, Cipher, Null.

```c
typedef enum { DigiviceAttrPortador, DigiviceAttrRuido, DigiviceAttrCifrado, DigiviceAttrNulo, DigiviceAttrCount } DigiviceAttr;
```

`digivice_rules_attr_adv(attacker, defender)` returns +1 when the attacker beats the defender, -1 when the defender beats the attacker, 0 otherwise (including every Nulo pairing and same-vs-same).

| Form | Stage | Attribute | Reason |
|---|---|---|---|
| Byteegg, Bitling, Bytemon | Egg, Baby, Rookie | Nulo | Undeclared forms are hollow and neutral |
| Wavedrake (Sub-GHz) | Champion | Portador | Radio waves |
| Tagdrake (NFC) | Champion | Cifrado | Tags and keys |
| Beamdrake (IR) | Champion | Ruido | Bursty, dirty light |
| Plugdrake (USB) | Champion | Nulo | A wire is hollow, with no radio |
| Virubyte | stage of `prev_form` | Ruido | The failure form is static |

VIRUX (Hacker mode) takes its attribute from its data type with the same mapping: NFC to Cifrado, Sub-GHz to Portador, IR to Ruido, USB to Nulo.

## Battle rules (pure C, `digivice_battle.c`)

### Power

| Stage | Base power `P0` |
|---|---|
| Egg, Baby | 0 (Egg cannot fight) and 8 (Baby fights trail foes; see "Who can fight") |
| Rookie | 20 |
| Champion | 40 |

- Player power `P = P0 + min(strength, 24)`, then x3/4 (integer) while injured. Champion range is 40 to 64, Rookie 20 to 44.
- Charge tier multiplies the **player's** power for that shot only: Small x0.6, Normal x1.0, Big x1.5 (stored x10: 6, 10, 15).
- Foe power `Q` is fixed per encounter (see Expedition for trail foes and bosses). VIRUX `Q`: Rookie 24, Champion 46.

### Hit chance

```
pct = 100*P / (P+Q)  (rounded to nearest)
pct += 5*adv         (adv from the shooter's attribute vs the target's)
pct  = clamp(pct, 10, 90)
if the shot is blocked: pct = max(5, pct / 2)
```

`digivice_rules_hit_pct(p, q, adv)` returns the clamped value. Examples:

| Shooter | P | Q | adv | pct |
|---|---|---|---|---|
| Rookie, no strength, Normal | 20 | 10 (area 1 trail foe) | 0 | 67 |
| Rookie, no strength, Big | 30 | 10 | 0 | 75 |
| Champion Wavedrake, strength 24, Normal | 64 | 59 (Ruido foe, area 5) | +1 | 57 |
| Foe vs Champion 64 | 59 | 64 | -1 (Ruido vs Portador is foe's disadvantage) | 43 |
| Champion 64 vs area 8 boss | 64 | 131 | 0 | 33 |

### Player turn

1. **Charge.** A 20-segment meter (`DIGIVICE_METER_SEGMENTS`) ping-pongs one segment per 100 ms frame. A zone of width `w = 3 + min(strength,24)/8` (3 to 6) sits at a random start in `[3, 20-w-1]`. OK stops the cursor. If OK is not pressed in 30 frames the cursor stops where it is.
   - Tier: cursor inside the zone is **Big**; within 2 segments of the zone is **Normal**; otherwise **Small**.
   - `digivice_rules_charge_tier(cursor, zone_start, zone_w)` and `digivice_rules_charge_zone_w(strength)` are pure.
2. **Aim.** The foe guard shows a shield at the high or the low slot and flips every 5 frames, starting from a random side `g0`. Up or Down chooses the aim. If neither is pressed in 30 frames the aim is High.
   - `digivice_rules_guard_at(g0, frame)` = `g0 ^ ((frame / 5) & 1)`.
   - The guard is sampled at the impact, 5 frames after the press: `blocked = (aim == guard_at(g0, press_frame + 5))`. A player who reads the rhythm can aim away from it.
3. **Resolve.** Roll `rng_pct < pct`. A pet with corruption of at least 50 (`digivice_rules_glitchy`) first rolls the existing 25% corruption miss, which turns the shot into a miss before the hit roll.
4. Outcome of a shot, which the UI needs for the animation:

| blocked | roll | Result | Animation |
|---|---|---|---|
| no | hit | HIT | shot reaches the foe, hit effect |
| no | miss | DODGE | foe hops, shot flies off the edge |
| yes | hit | GUARD BROKEN | shield bursts, then the hit effect |
| yes | miss | BLOCKED | shot dies on the shield |

### Foe turn

Automatic. The foe picks a random high or low height for the animation only. Hit chance is `100*Q/(P+Q) + 5*adv_foe`, clamped, using the player's **uncharged** power. If the player holds the Shield and it is unspent, the first foe hit is cancelled (shown as "EL ESCUDO AGUANTA") and the Shield is consumed. A glitchy pet gains nothing from this.

### Flow and outcome

- `DigiviceBattle` holds attributes, powers, shot sizes, `player_hits`, `foe_hits`, `round`, `shield`.
- Rounds alternate: player shot, then foe shot. After each shot, check: 3 hits ends the battle at once.
- **First to 3 hits wins.** After round 10 (`DIGIVICE_ROUND_CAP`) with nobody at 3: more hits wins, equal hits is `DigiviceBattleDrawn`.
- `digivice_battle_outcome` returns `Ongoing`, `Won`, `Lost` or `Drawn`.

### Who can fight

`digivice_rules_can_battle` now refuses with `DigiviceResultSick`, `DigiviceResultAsleep`, or the new `DigiviceResultTooYoung` (stage below Baby). Only the Egg cannot battle. The Baby can fight trail foes (user decision: fast start, the Baby fights weak foes), but VIRUX stay Rookie+ and bosses stay Champion-gated.

### Retreat

`digivice_battle_retreat_allowed(state, kind)`:

| Encounter | Retreat (Back during Charge or Aim) |
|---|---|
| VIRUX (Hacker) | Allowed |
| Trail fight in areas 1 to 4 | Allowed |
| Boss in areas 1 to 4 | Allowed (the boss stays at distance 0) |
| Anything in areas 5 to 8 | **Refused** with "¡NO HAY ESCAPE!" |

A retreat is `DigiviceBattleResultFled`: no EXP, no penalty, the pending trail encounter is consumed, the boss stays available. Declining **before** the first Charge (Back during the encounter transition) is always allowed, even in streak areas, and costs nothing: the encounter stays pending until it expires.

### Results

`digivice_rules_battle_result(s, kind, result, rng_pct, now)` replaces the v1 function and returns a `DigiviceReward { uint16_t exp; uint8_t item; }`.

| Result | Effect |
|---|---|
| Won, trail | `wins++`, EXP = `6 + 2*(area+1)` (8 to 22). 10% chance (`rng_pct < 10`) of a Shield if none is held |
| Won, boss | `wins++`, EXP = `40 + 20*(area+1)` (60 to 200), halved on a repeat clear. Shield guaranteed if none is held. Marks the area cleared and advances (see Boss) |
| Won, VIRUX | v1 reward: `+2*DIGIVICE_WIN_BASE_EXP` (20) EXP, `+3` data of the VIRUX type, `wins++` |
| Lost | `losses++`, corruption +10 (existing `DIGIVICE_CORRUPTION_LOSE_GAIN`), mood -1, injury for 3600 s. In areas 5 to 8 (trail or boss) distance resets to the area length and `mid_state` to 0 |
| Drawn, Fled | Nothing changes except the consumed trail encounter or VIRUX |

A trail or VIRUX encounter is cleared on any result. A used Shield is cleared on any result that consumed it.

## Expedition rules (pure C, `digivice_expedition.c`)

### Area table

Lengths are set so the average area takes about one awake day: an awake day is 07:00 to 23:00, so 16 h x 60 = 960 paces, and the lengths average 915. Area 1 is half a day, area 8 a day and a half. Total 7,320 paces, about 7.6 awake days at 1 pace/min with no Hike. Hike and scans shorten this (see below).

Area power and boss power: trail foe `Q = area_power x tier`, where Rookie-tier foes use x1.0 and Champion-tier foes x1.3. Boss `Q = area_power x 1.4`, integer floor.

| # | Name ES / EN | Length | Midway at | Midway kind | Area power | Boss `Q` | Boss (attr) | Trail pool (attr) | Retreat |
|---|---|---|---|---|---|---|---|---|---|
| 1 | PRADO ESTÁTICO / STATIC MEADOW | 480 | 240 | Spring | 10 | 14 | CHISPAPO (Ruido) | ESTATIK (R), KORNO (P) | yes |
| 2 | ARBOLEDA COBRE / COPPER GROVE | 600 | 300 | Cache | 16 | 22 | CABLEBOA (Cifrado) | CHIPKO (C), ESTATIK (R) | yes |
| 3 | CRESTA ANTENA / ANTENNA RIDGE | 720 | 360 | Shortcut | 24 | 33 | MASTIRAM (Portador) | KORNO (P), CHIPKO (C) | yes |
| 4 | CACHÉ HELADO / FROZEN CACHE | 840 | 420 | Spring | 34 | 47 | CRISTALOK (Cifrado) | MOKOBIT (N), CHIPKO (C) | yes |
| 5 | DUNAS IMÁN / MAGNET DUNES | 960 | 480 | Cache | 46 | 64 | GUSAFERRO (Ruido) | FUZZCAN (R), KORNO (P) | **streak** |
| 6 | TORRE RELÉ / RELAY TOWER | 1080 | 540 | Shortcut | 60 | 84 | CENTINELA (Cifrado) | VAMPIXEL (R), CHIPKO (C) | **streak** |
| 7 | BÚFER HONDO / DEEP BUFFER | 1200 | 600 | Spring | 76 | 106 | DESBORDE (Ruido) | SILBAPO (R), MOKOBIT (N) | **streak** |
| 8 | NÚCLEO NULO / NULL CORE | 1440 | 720 | Cache | 94 | 131 | NEGATIVO (Nulo) | CHATARRO (N), FUZZCAN (R) | **streak** |

Tiers: ESTATIK, CHIPKO, KORNO and MOKOBIT are Rookie-tier. FUZZCAN, VAMPIXEL, SILBAPO and CHATARRO are Champion-tier. Resulting trail `Q` by area: 10, 16, 24, 34, then 46/59, 60/78, 76/98 and 94/122 for the Champion-tier foes.

Balance check with the current forms. A Rookie at strength 24 (P 44) has about 56% against an area 4 trail foe. A Champion at strength 24 (P 64) has 48% against the area 5 boss and 33% against the area 8 boss: tough, and that is the design until v3.2 adds stages above Champion. Shield, Big charges and attribute advantage are the player's levers.

Areas unlock in order: area `i` is selectable when `i == 0` or boss `i-1` is cleared (`cleared` bit set).

### Paces

- **1 pace per awake minute** (`DIGIVICE_PACE_S` = 60). `pace_acc_s` carries awake seconds like `hunger_acc_s`. The existing `awake_advance(s, step, t_end)` in `digivice_rules.c` calls `digivice_expedition_walk(s, step, t_end)`, so the offline catch-up, the 48 h clamp and sleep behave exactly like the rest of the rules.
- A pace counts only if `stage >= Baby`, `!sick` and `distance > 0`. Sleep (23:00 to 07:00) is excluded because `awake_advance` is never called during it.
- When `pace_acc_s` crosses 60, each pace gets its exact instant: `t_end - (acc_after_add - 60*k)`. That instant drives encounter expiry, so a long offline stretch ends with only the latest encounter pending, or none.
- Each pace: `distance--`, `paces_since_fight++`, midway check, then the encounter roll.
- At `distance == 0` paces stop (no more rolls). The boss is "ready".

### Hike (rhythm-tap minigame)

- Session: 24 beats (`DIGIVICE_HIKE_BEATS`), one every 500 ms (`DIGIVICE_HIKE_BEAT_MS`), 12 s long. A tap is good when it lands within 150 ms (`DIGIVICE_HIKE_GOOD_MS`) of a beat. Each beat can pay once. A bad or extra tap does nothing.
- `digivice_expedition_hike_tap_good(phase_ms)` is the pure judge: `phase_ms` is the signed distance to the nearest beat.
- At the end (or on Back) the app sends `good_taps` once. `digivice_expedition_hike(s, good_taps, now)` grants `min(good_taps, 60 - hike_count)` paces and returns the granted number.
- **Cap: 60 paces per hour**, in a rolling window: if `now - bonus_window_start >= 3600` the window restarts at `now` and `hike_count` and `scan_count` reset to 0.
- Each granted pace goes through the same pace routine (distance, midway, encounter roll) at instant `now`.

### Hacker scans

- In Hacker mode, each read or scan deed (the existing `digivice_rules_is_spawn_deed` set) grants **3 paces** (`DIGIVICE_SCAN_PACES`), capped at **30 paces per hour** in the same rolling window (`scan_count`). Same stage and sick conditions as normal paces.
- The existing VIRUX roll on these deeds (20%, Rookie or above, none active) is unchanged. A VIRUX encounter and a trail encounter never coexist.
- Normal mode still ignores deeds: no scan paces.
- Ceiling per hour: 60 idle + 60 Hike + 30 scans = 150 paces.

### Encounters

- Each pace rolls `rng % 100 < 3` (`DIGIVICE_ENCOUNTER_PCT`), or fires for certain when `paces_since_fight >= 50` (`DIGIVICE_ENCOUNTER_GUARANTEE`). The roll only happens when no trail encounter is pending and no VIRUX is active.
- On a spawn: `enc_foe = rng % 2` (index into the area's pool of 2), `enc_expires = t + 1800` (`DIGIVICE_ENCOUNTER_TTL_S`), `paces_since_fight = 0`.
- **Expiry:** a pending encounter whose `enc_expires` has passed is cleared at tick time and at pace time. No penalty. The service tick also clears it.
- Pending VIRUX keeps its own 30 min TTL (`DIGIVICE_VIRUX_TTL_S`).
- The expected rate is about one encounter per 33 paces, so a 480 pace area sees about 14 spawns. Fights are optional because skipping is free.
- `digivice_rules_attention(s, now)` is true when a trail encounter or a VIRUX is pending, when midway is available (`mid_state == 1`), or when the boss is ready and the gate is open. The home screens use it for the "!" bubble.
- **Notification:** a new spawn (trail or boss ready) plays the LED and vibration sequence unless asleep. Spawns recreated during an offline catch-up notify only if still pending.

### Midway

`mid_state`: 0 not reached, 1 available, 2 done. It becomes 1 when `distance <= length/2` (once, never reverted except by a streak reset). The event is chosen by the area table and claimed from the Expedition screen. It never expires and never blocks paces.

| Kind | Effect |
|---|---|
| Spring | Hunger and mood to full, corruption -10, injury cleared |
| Cache | A Shield if none is held, otherwise +20 EXP (`DIGIVICE_CACHE_EXP`) |
| Shortcut | The player chooses OK to try or Back to skip. Try: `rng_pct < 50` gives `distance -= length/5`; otherwise `distance += length/5`, capped at the area length. Skip: nothing. The distance may reach 0 (boss ready) |

`digivice_expedition_midway_claim(s, accept, rng_pct, now)` returns a `DigiviceMidwayResult { kind, ok }` and sets `mid_state = 2`.

### Boss and gate

- Boss ready = `distance == 0`. It never expires.
- **Gate:** the fight needs `stage >= Champion` (Virubyte uses the stage of `prev_form`). Below that, the app shows the gate screen: "NECESITAS UN CAMPEÓN", options RETREAT (back to the main screen) or TRAIN (opens training). Progress is kept.
- `digivice_expedition_boss_gate_open(s)` is pure.
- **Win:** set the area's `cleared` bit, then move to the lowest uncleared area (distance = its length, `mid_state = 0`, `paces_since_fight = 0`). If all 8 are cleared, stay at area 8 with `distance 0` and show "¡TODAS LAS ZONAS!".
- **Loss:** areas 1 to 4 keep distance 0. Areas 5 to 8 reset distance and midway (streak lost).
- **Streak areas (5 to 8):** losing any fight (trail or boss) resets `distance` to the area length, `mid_state` to 0 and `paces_since_fight` to 0. Ignoring an encounter does not count as a loss.

### Map and area switching

`digivice_expedition_select_area(s, area)` returns false when the area is locked. Otherwise, if it differs from `s->area`: `area = n`, `distance = length`, `mid_state = 0`, `enc_foe = none`, `paces_since_fight = 0`. Selecting the current area changes nothing.

## Persisted state (v3) and migration

`DigiviceState` gains these fields after the v2 ones (offset 84), all fixed width:

| Field | Type | Meaning |
|---|---|---|
| `area` | `uint8_t` | 0..7, the active area |
| `cleared` | `uint8_t` | bitmask of defeated bosses |
| `enc_foe` | `uint8_t` | pending trail foe, index 0 or 1 in the pool, `0xFF` none |
| `mid_state` | `uint8_t` | 0 not reached, 1 available, 2 done |
| `item` | `uint8_t` | 0 none, 1 Shield |
| `hike_count` | `uint8_t` | Hike paces in the current window, 0..60 |
| `scan_count` | `uint8_t` | scan paces in the current window, 0..30 |
| `reserved3` | `uint8_t` | padding |
| `distance` | `uint16_t` | paces left in the area, 0..length |
| `paces_since_fight` | `uint16_t` | 0..50 |
| `pace_acc_s` | `uint32_t` | awake seconds toward the next pace |
| `enc_expires` | `uint32_t` | timestamp, 0 when none |
| `bonus_window_start` | `uint32_t` | start of the Hike and scan window, 0 when none |
| `injured_until` | `uint32_t` | timestamp, 0 when healthy |
| `rng` | `uint32_t` | xorshift32 state, never 0 |

`sizeof(DigiviceState)` goes 84 to **116** bytes. A `static_assert(offsetof(DigiviceState, area) == sizeof(DigiviceStateV2))` guards the prefix.

- The saved_struct version goes from `0x02` to `0x03`. The magic stays.
- **Types.** `DigiviceStateV2` is an exact copy of the v2 layout (84 bytes), kept in `digivice_rules.h`. `DigiviceStateV1` stays as is.
- **Migration chain** in `digivice_load_file`:
  - `0x03` with `size == sizeof(DigiviceState)`: load normally.
  - `0x02` with `size == sizeof(DigiviceStateV2)`: load into a V2 struct, `digivice_rules_migrate_v2(const DigiviceStateV2*, DigiviceState*)`.
  - `0x01` with `size == sizeof(DigiviceStateV1)`: load V1, `digivice_rules_migrate_v1(const DigiviceStateV1*, DigiviceStateV2*)` (now outputs V2), then `migrate_v2`.
  - Anything else: a fresh egg.
- `migrate_v2` copies the 84-byte prefix and sets: `area = 0`, `cleared = 0`, `enc_foe = 0xFF`, `mid_state = 0`, `item = 0`, counters 0, `distance = length of area 1` (480), `pace_acc_s = 0`, `enc_expires = 0`, `bonus_window_start = 0`, `injured_until = 0`, `rng = (last_tick ^ born_at ^ 0x9E3779B9) | 1`. It leaves lang, mode, care state and any pending VIRUX untouched.
- **Re-anchor rule (kept).** When a file was migrated (from v1 or v2), the service sets `last_tick = now` after loading and marks the state dirty. The first tick then sees about 0 s elapsed, so the old pet earns no free paces or care penalties for the time spent under the old firmware.
- `digivice_rules_sanitize` also clamps: `area <= 7`, `cleared` to 8 bits, `enc_foe` in `{0, 1, 0xFF}`, `mid_state <= 2`, `item <= 1`, `distance <= length(area)`, `hike_count <= 60`, `scan_count <= 30`, `paces_since_fight <= 50`, `rng != 0`. If both a VIRUX and a trail encounter are pending, the trail one is cleared.
- `digivice_rules_same_visible` also masks `pace_acc_s`, `paces_since_fight`, `rng`, `hike_count`, `scan_count` and `bonus_window_start`, and compares `distance` only as `distance == 0`. A midway reaching 1, a spawn, an item, an injury, or the boss becoming ready are all visible.

## Service changes (`digivice_srv.c`, `digivice.h`)

- Before every tick and every deed, the service mixes entropy into the state: `state->rng ^= furi_hal_random_get()`, and sets it to 1 if it becomes 0. All pure code then uses `s->rng` through xorshift32, so host tests stay deterministic by setting `s->rng` directly.
- The tick calls `digivice_rules_tick` as today (which now includes paces and expiry) and then `digivice_rules_evolve`.
- Public calls, all blocking like the existing ones:
  - `DigiviceReward digivice_battle_finish(Digivice*, DigiviceEncounterKind kind, DigiviceBattleResult result)` replaces the v1 call. It draws two `rng_pct` values itself (item drop, shortcut-style rolls).
  - `uint8_t digivice_hike_finish(Digivice*, uint8_t good_taps)` returns the paces granted and marks the state dirty explicitly (the hike changes only bookkeeping fields).
  - `bool digivice_select_area(Digivice*, uint8_t area)`.
  - `DigiviceMidwayResult digivice_midway_claim(Digivice*, bool accept)`.
- `digivice_changed` gets new notifications, both silent while asleep: `digivice_sequence_encounter` (LED and vibration) when `enc_foe` goes from none to set or `enc_expires` changes while set, and the same sequence when `distance` goes from above 0 to 0. Two new sequences are added in `digivice_notify.c`: `digivice_sequence_encounter` and `digivice_sequence_lose`. Charge, hit and win sequences already exist and are reused.
- `DigiviceMessage` gains `DigiviceMessageTypeHike`, `SelectArea` and `Midway`, plus an output pointer for the reward and midway result. The queue size (16) is unchanged.
- The load path implements the chain above, with at most the V1 (56 B) and V2 (84 B) temporaries on the stack at the same time.
- The existing Hacker deed path (`digivice_rules_on_deed`) calls `digivice_expedition_scan(s, now)` after its current effects. In Normal mode nothing changes.

## App (`applications/main/digivice/`)

### Screens and flow

New and changed screens, in `DigiviceScreen`:

| Screen | Purpose |
|---|---|
| `Expedition` | The trail view (replaces the v1 BATALLA entry) |
| `Map` | Area picker |
| `Hike` | Rhythm minigame |
| `Midway` | Spring, cache and shortcut event screens |
| `Gate` | Boss gate |
| `Encounter` | Transition and intro (trail, boss, VIRUX) |
| `Battle` | The new battle (rewritten `digivice_app_battle.c`) |

The new file is `digivice_app_expedition.c` (Expedition, Map, Hike, Midway and Gate).

- **Main screen:** the third icon opens `Expedition` in both modes. Refusals: sick (existing notice), asleep (existing notice), too young ("CRECE PARA LUCHAR") only apply when starting a fight, not to opening the trail view.
- **Auto-start.** When the app opens and an encounter is pending, it goes straight to `Encounter` instead of the main screen. Priority: pending trail or VIRUX first, then a ready boss with an open gate. Midway and a gated boss do not auto-start. The app remembers, in RAM only, which encounter it already auto-started or declined (`enc_expires` or `virux_expires`, plus kind), so it does not retry after a decline; opening the trail view and pressing OK starts it on demand.
- **State refresh.** On the Expedition and Map screens the app copies the service state once per second (`digivice_get_state`). Distance is bookkeeping and is not published, so the app cannot rely on pubsub for it.

### Expedition screen (from the mockup `trail` scene)

- Top bar: area name left, `D` plus remaining paces right (for example `D0312`).
- Progress line at y 30 from x 8 to 116: pace marker moves with `(len-distance)/len`; a flag at the midway point (hollow before, filled after), a skull at the end, the pet marker walking.
- Left column: `PASOS done/len` and `RESTAN n` (numbers drawn by the app, labels from the string table). Right column: "1 PASO/MIN" and "DESPIERTO".
- A Shield icon next to the top bar when `item == 1`; a bandage when injured.
- Footer panel (y 51): next landmark ("PRÓX: FUENTE", "PRÓX: COFRE", "PRÓX: ATAJO" or "PRÓX: JEFE") and a hint line.
- **Controls:** OK does the first applicable action: pending encounter (start battle), ready boss (gate or boss intro), available midway (event), else Hike. Up opens the Map. Back returns to the main screen.
- When the pet cannot walk, the hint line says why ("ENFERMO: NO CAMINA", "DUERME: NO CAMINA", "CRECE PARA CAMINAR") and the background does not scroll.

### Map screen (from the mockup `map` scene)

- Eight nodes on a zig-zag at fixed coordinates `(10,44) (25,25) (41,44) (56,23) (72,43) (88,22) (104,40) (118,20)`. Done nodes show a check, the current one blinks, locked ones show `?`, streak areas (5 to 8) get corner brackets.
- Left and Right move the selection (bracket cursor). OK selects an unlocked area, Back leaves.
- Footer: `n NAME` with HECHO, AQUÍ or BLOQ., and `LEN PASOS` with RACHA or RETIRO OK.

### Hike screen (from the mockup `trail` scene, bottom half)

- Notes scroll left at 4 px per frame, one every 5 frames (20 px apart). The target box is at x 60 to 65, y 57 to 62. OK taps are judged against the nearest beat by `furi_get_tick`.
- Good tap: the box fills for 2 frames, "+1", LED green blink. The counter "n/24" and the hour budget "x/60" are shown.
- At the end: "PASOS GANADOS n". If the hour cap is reached the screen says "LÍMITE POR HORA" and no session starts.

### Encounter transition (from `areaScene` and `bossScene`)

Frames of the 100 ms animation clock, about 4 s in total. Back skips (decline) at any point before the first Charge.

| Frames | Trail and VIRUX | Boss |
|---|---|---|
| 0 to 8 | Exclamation bubble over the pet, LED red blink, 4 frames of vibration | Hazard tape bars scroll in at y 9 and y 56, "ALERTA" blinks, screen shakes with growing amplitude, LED red |
| 8 to 16 | Wipe in 16 horizontal strips, glitch strips, 2 inverted frames | Boss silhouette rises from below, one full inversion frame |
| 16 to 22 | VS banner wipes in (diagonal band, "VS" at 2x), LED in the foe attribute colour | Banner "JEFE", boss name and "ZONA n" |
| 22 to 29 | Pet slides in from the left, foe from the right (clamped) | Same |
| 29 to 40 | Typed message "¡UN <NAME> APARECE!" plus attribute icon, power line, and the edge label (VENTAJA, DESVENTAJA or NEUTRO) | Boss name, attribute icon, edge label |

Then the first Charge starts by itself. The hint "< HUIR" is shown while retreat is possible and "¡NO HAY ESCAPE!" is the notice on a refused Back.

### Battle screen (from `combate.html`)

Layout in screen pixels (128x64):

- HUD bar y 0 to 7, black: pet name left, foe name right, "VS" at x 57, three hit boxes per side (`x = 32 + i*6` for the player, `x = 87 - i*6` for the foe), filled when scored.
- Arena band y 11 to 50, dotted background; shadow ellipses at x 30 and x 98, y 51. Pet feet at (30, 50), foe feet at (98, 50), foe mirrored.
- Panel y 51 to 63, two text lines at y 53 and y 59.

| Phase | Frames | What happens |
|---|---|---|
| Charge | up to 30 | Meter at y 59 (20 segments, 4 px wide, 5 px pitch, x 3 to 98), zone drawn hollow, cursor as a tall bar. FUERZA label and hearts top right. OK button glyph at x 109. Line: "¡CARGA! OK EN LA ZONA". After OK, the line shows the tier ("¡TIRO GRANDE!") and a dash ring around the mouth grows with the tier |
| Aim | up to 30 | Up and Down arrows at x 3 (y 14 and y 40) blink; the foe shield flickers high and low every 5 frames at x 70 (y 18 high, y 34 low). Line: "APUNTA: ^ ALTO  v BAJO". The pressed arrow stays filled |
| Shot (either side) | 5 wind-up, 4 travel, 5 impact | Attacker uses its attack pose and shows a charging dot ring; the projectile moves from the mouth to the target (6 px higher for High, lower for Low); impact effects below |
| Result line | during impact | "¡GOLPE! n/3", "¡BLOQUEADO!", "¡GUARDIA ROTA!", "¡ESQUIVA!" plus "EL TIRO SE PIERDE"; foe turn shows "TURNO RIVAL" and "¡TE DIO!" or "¡FALLÓ!" |

**Projectile size by stage:** Rookie 3x3, Champion 5x5, boss 7x7, area 8 boss a 9x5 beam. Foes use their tier (Rookie-tier 3x3, Champion-tier 5x5); VIRUX follows the pet's stage. The style follows the shooter's attribute: Portador is three concentric arcs (a ring wave), Ruido a jagged two-pixel bolt, Cifrado a hollow square packet with a blinking trail, Nulo a hollow circle. The player's Big tier adds one pixel to the radius.

**Hit effect** (target takes a hit): impact frames 0 and 1 invert the target's bounding box (XOR fill), a 5x5 burst star and a radial burst grow over frames 0 to 4, the arena layer shakes 2 px on frame 0 and 1 px on frames 1 to 3 (the HUD never shakes), `digivice_sequence_hit` plays (LED red, 40 ms vibration), and a "-1" floats up in a small box. **Miss:** the target hops up to 10 px in a sine arc (dodge), the shot continues off the edge with three speed lines, no flash, no vibration.

**Result screens:**

- **Victory:** foe fades (dither) in 12 frames, pet happy with sparkles, a "¡VICTORIA!" banner drops from the top (y -14 to 10, 5 frames), "+n EXP" counts up, then, if a Shield dropped, an item bounces in and the line shows "OBJETO: ESCUDO". LED green, `digivice_sequence_win`.
- **Defeat:** the pet turns sad and sinks 2 px, "DERROTA" slides in, a bandage appears with a small burst, line "¡HERIDO! -1 ÁNIMO". In areas 5 to 8 a second line says "¡RACHA ROTA! VUELVES AL INICIO". LED red, `digivice_sequence_lose`.
- **Draw:** "EMPATE: EL RIVAL HUYE", neutral pose, no LED.
- **Retreat:** "HUISTE SIN DAÑO".
- **Boss win:** after the victory banner, "¡ZONA COMPLETADA!" for 12 frames and the app returns to the Expedition screen showing the new area.

After any result, OK or Back returns to the Expedition screen. The service call `digivice_battle_finish` is made once, at the moment the result is decided (before the result animation), so a power loss during the animation does not lose it. If the encounter expired mid-fight the call still applies the result; clearing a missing encounter is a no-op.

### Controls summary

| Context | OK | Up/Down | Left | Back |
|---|---|---|---|---|
| Expedition | Context action | Up: Map | - | Main |
| Map | Select area | - | Left/Right: move | Expedition |
| Hike | Tap | - | - | End session |
| Midway Shortcut | Try | - | - | Skip |
| Gate | - | - | Left: RETREAT, Right: TRAIN | Main |
| Encounter transition | - | - | - | Decline (always allowed) |
| Battle Charge | Stop the gauge | - | - | Retreat (if allowed) |
| Battle Aim | - | Up: High, Down: Low | - | Retreat (if allowed) |
| Result | Continue | - | - | Continue |

### Desktop home view

`desktop_view_digivice.c` replaces the VIRUX-only bubble with `digivice_rules_attention`: the same blinking "!" bubble, and the corner label shows "VIRUX" with its countdown when a VIRUX is active, `ENCUENTRO / ENCOUNTER` for a trail encounter, and `¡JEFE! / BOSS!` for a ready boss. Strings go through `digivice_str` with the state's `lang`.

## Draw kit (`digivice_draw.h`, new `digivice_draw_world.c`)

New functions, all clipped like the existing `dv_*` helpers:

| Function | Notes |
|---|---|
| `digivice_draw_foe(canvas, DigiviceFoe, cx, foot_y, frame, pose, flip, silhouette)` | 8 trail monsters. About 24x24 for Rookie-tier, 32x32 for Champion-tier, built from ellipses, triangles and lines. Idle bob, attack pose (mouth open), hurt pose |
| `digivice_draw_boss(canvas, area, cx, foot_y, frame, silhouette)` | 8 bosses, each at most 56x48 px (the mockup's 80x60 does not fit beside the pet), from the mockup `BOSS` functions simplified. NEGATIVO is the player's own silhouette drawn inverted |
| `digivice_draw_shot(canvas, attr, x, y, size, dir, frame)` | The four projectile styles at sizes 3, 5, 7 and the 9x5 beam |
| `digivice_draw_attr_icon(canvas, attr, x, y)` | 7x7 icons: wave, bolt, keyhole-square, hollow circle |
| `digivice_draw_burst`, `digivice_draw_star5`, `digivice_draw_shield`, `digivice_draw_exclam_bubble`, `digivice_draw_bandage` | Small effects from the mockup |
| `digivice_draw_invert(canvas, x, y, w, h)` | XOR box, used for the hit flash and strobe frames |
| `digivice_scene_init(DigiviceScene*, area)` and `digivice_draw_scene(canvas, const DigiviceScene*, frame, hour)` | Procedural area backgrounds, below |

`digivice_form_name` and `digivice_type_name` gain `digivice_attr_name(attr, lang)` (translated, through i18n). `digivice_draw_form` is used unchanged for the pet.

### Seeded backgrounds

The 8 area backgrounds port the mockup's `bgMeadow`, `bgGrove`, `bgRidge`, `bgIce`, `bgDunes`, `bgRelay`, `bgDeep` and `bgNull`. The mockup draws them with per-pixel hashing and recomputes everything each frame, which the device cannot afford. The device version is cheap by construction:

- **Seed.** `seed = 0xD1610 ^ (area * 0x9E37)`, a constant per area. The seed is not persisted: the same area looks the same on every run.
- **Cache.** `DigiviceScene` (about 420 bytes) is allocated on the app heap when the Expedition, Encounter or Boss screen opens and freed when leaving. It holds two height profiles of 192 px each (`far[192]`, `near[192]`, value noise generated once from the seed with a xorshift and interpolation every 16 px) and 12 decoration slots (x position and variant). Generation runs once per area change, about 400 integer operations.
- **Per frame.** Indexing a profile by `(x + scroll) % 192` and drawing one vertical line per column gives about 256 calls for two layers, plus at most 12 decorations (trees, masts, ice blocks, dunes) only when on screen. Texture fills use the existing 4x4 ordered dither (`digivice_draw_dither`) over rectangles, with the phase shifted by `frame` where the mockup used animated noise (the meadow snow). **No per-pixel hash per frame.** Target: at most 350 canvas calls and at most 30 ms of draw time per 100 ms frame on the device (the check list measures it).
- **Scroll.** 1 px per frame while the pet is walking on the Expedition screen. It freezes when the pet cannot walk, in battle (the arena is the dotted band, not the area), and in the gate.
- **Day and night.** `hour` comes from the RTC, read once per second. Three bands: day 07:30 to 18:00, dusk 18:00 to 20:00 and 06:00 to 07:30, night 20:00 to 06:00. The sky is drawn per band: sun arc and 3 cloud blobs by day, a low sun and a light dither gradient at dusk, a dither-filled sky with 26 hashed stars and a crescent at night (stars use the cached seed, not a per-frame hash). At night the layer fills invert (dark ground, light outlines), as in the mockup's generator. The pet only sleeps from 23:00, so the 20:00 to 23:00 evening and the morning dusk are the visible cases.
- The tower strobe (area 6) and the mast pulses (area 3) are single-frame toggles on the 100 ms clock.

### Trail monsters and bosses

| Foe | Attr | Tier | Look (from the mockup) |
|---|---|---|---|
| ESTATIK | Ruido | Rookie | Imp with static hair, broken wings, arrow tail |
| CHIPKO | Cifrado | Rookie | Square gecko with chip pins on its back |
| KORNO | Portador | Rookie | Puppy with one horn, a drooping ear and a wave on its chest |
| MOKOBIT | Nulo | Rookie | Hollow blob with bubbles |
| FUZZCAN | Ruido | Champion | Bristly hound with TV noise, bent ear |
| VAMPIXEL | Ruido | Champion | Dark-winged bat that glitches |
| SILBAPO | Ruido | Champion | Fat toad with warts and an inflating sac |
| CHATARRO | Nulo | Champion | Hollow scrap golem with mismatched arms |

Bosses by area: CHISPAPO (bloated crackle toad), CABLEBOA (coiled cable serpent), MASTIRAM (mast-backed ram), CRISTALOK (crystalline vault beetle), GUSAFERRO (iron-filings sand wurm), CENTINELA (strobing sentinel), DESBORDE (overflow leviathan), NEGATIVO (negative-space silhouette of the player's pet).

## Strings (i18n)

Every new string exists in Spanish and English. `digivice_i18n_budget` gets an entry for each. Budgets are expressed in glyphs of the slot, where `px = 4*glyphs - 1`:

| Slot class | Glyphs | Px |
|---|---|---|
| T14 top bar | 14 | 55 |
| A8 attribute name | 8 | 31 |
| N22 notice box | 22 | 87 |
| H12 right column | 12 | 47 |
| P30 panel line | 30 | 119 |
| P26 beside the OK button | 26 | 103 |
| P22 beside an attribute icon | 22 | 87 |
| B10 banner or option | 10 | 39 |

Strings with numbers (paces, hits, EXP, distance, area number) are split into a label string and a number drawn by the app: there are no printf formats in the tables. A foe name is concatenated between two string fragments.

| ID | ES | EN | Slot |
|---|---|---|---|
| `MenuBattle` (replaces BATALLA) | RUTA | TRAIL | existing menu slot |
| `AttrPortador` | PORTADOR | CARRIER | A8 |
| `AttrRuido` | RUIDO | NOISE | A8 |
| `AttrCifrado` | CIFRADO | CIPHER | A8 |
| `AttrNulo` | NULO | NULL | A8 |
| `Area1` to `Area8` | PRADO ESTÁTICO, ARBOLEDA COBRE, CRESTA ANTENA, CACHÉ HELADO, DUNAS IMÁN, TORRE RELÉ, BÚFER HONDO, NÚCLEO NULO | STATIC MEADOW, COPPER GROVE, ANTENNA RIDGE, FROZEN CACHE, MAGNET DUNES, RELAY TOWER, DEEP BUFFER, NULL CORE | T14 |
| `ExpTitle` | EXPEDICIÓN | EXPEDITION | T14 |
| `ExpPaces` | PASOS | PACES | B10 |
| `ExpLeft` | RESTAN | LEFT | B10 |
| `ExpRate` | 1 PASO/MIN | 1 PACE/MIN | H12 |
| `ExpAwake` | DESPIERTO | AWAKE | H12 |
| `ExpNextSpring` | PRÓX: FUENTE | NEXT: SPRING | P30 |
| `ExpNextCache` | PRÓX: COFRE | NEXT: CACHE | P30 |
| `ExpNextShortcut` | PRÓX: ATAJO | NEXT: SHORTCUT | P30 |
| `ExpNextBoss` | PRÓX: JEFE | NEXT: BOSS | P30 |
| `ExpHintIdle` | OK: CAMINATA  ^: MAPA | OK: HIKE  ^: MAP | P30 |
| `ExpHintFight` | OK: LUCHAR | OK: FIGHT | P30 |
| `ExpHintMidway` | OK: EVENTO | OK: EVENT | P30 |
| `ExpHintBoss` | OK: JEFE | OK: BOSS | P30 |
| `ExpHintGate` | JEFE: NECESITA CAMPEÓN | BOSS: NEEDS CHAMPION | P30 |
| `ExpIdleSick` | ENFERMO: NO CAMINA | SICK: NOT WALKING | P30 |
| `ExpIdleAsleep` | DUERME: NO CAMINA | ASLEEP: NOT WALKING | P30 |
| `ExpIdleYoung` | CRECE PARA CAMINAR | GROW UP TO WALK | P30 |
| `ExpAllClear` | ¡TODAS LAS ZONAS! | ALL AREAS CLEAR! | P30 |
| `MapTitle` | MAPA | MAP | T14 |
| `MapDone` | HECHO | DONE | H12 |
| `MapHere` | AQUÍ | HERE | H12 |
| `MapLocked` | BLOQ. | LOCKED | H12 |
| `MapStreak` | RACHA | STREAK | H12 |
| `MapRetreatOk` | RETIRO OK | RETREAT OK | H12 |
| `HikeTitle` | CAMINATA | HIKE | T14 |
| `HikeHint` | ¡OK AL RITMO! | TAP TO THE BEAT! | P30 |
| `HikeCapped` | LÍMITE POR HORA | HOURLY LIMIT | P30 |
| `HikeGained` | PASOS GANADOS | PACES GAINED | P22 |
| `MidTitleSpring` | FUENTE | SPRING | T14 |
| `MidTitleCache` | COFRE | CACHE | T14 |
| `MidTitleShortcut` | ATAJO | SHORTCUT | T14 |
| `MidSpring` | ¡FUENTE! TODO RECUPERADO | SPRING! FULLY RESTED | P30 |
| `MidCacheShield` | ¡COFRE! HALLASTE UN ESCUDO | CACHE! FOUND A SHIELD | P30 |
| `MidCacheExp` | ¡COFRE! TE DA EXP | CACHE! GIVES EXP | P30 |
| `MidShortcutAsk` | ¡ATAJO ARRIESGADO! | RISKY SHORTCUT! | P30 |
| `MidShortcutKeys` | OK: PROBAR  <: SEGUIR | OK: TRY  <: SKIP | P30 |
| `MidShortcutWin` | ¡ATAJO! -20% CAMINO | SHORTCUT! -20% PATH | P30 |
| `MidShortcutLose` | ¡DESVÍO! +20% CAMINO | DETOUR! +20% PATH | P30 |
| `MidSkipped` | SIGUES TU CAMINO | ON YOU GO | P30 |
| `GateNeed` | NECESITAS UN CAMPEÓN | YOU NEED A CHAMPION | P30 |
| `GateScratch` | TU NOVATO SOLO RASGUÑA | YOUR ROOKIE ONLY SCRATCHES | P30 |
| `GateRetreat` | RETIRARSE | RETREAT | B10 |
| `EncWildPre` | ¡UN  | A WILD  | P22 |
| `EncWildPost` |  APARECE! |  APPEARS! | P22 |
| `EncBoss` | ¡JEFE! | BOSS! | B10 |
| `EncZone` | ZONA  | AREA  | B10 |
| `EncPow` | POT  | POW  | B10 |
| `EncEdge` | VENTAJA | ADVANTAGE | H12 |
| `EncDisedge` | DESVENTAJA | DISADVANTAGE | H12 |
| `EncNeutral` | NEUTRO | NEUTRAL | H12 |
| `EncRetreatHint` | < HUIR | < RETREAT | B10 |
| `BtlCharge` | ¡CARGA! OK EN LA ZONA | CHARGE! OK IN THE ZONE | P26 |
| `BtlShotSmall` | TIRO PEQUEÑO | SMALL SHOT | P26 |
| `BtlShotNormal` | TIRO NORMAL | NORMAL SHOT | P26 |
| `BtlShotBig` | ¡TIRO GRANDE! | BIG SHOT! | P26 |
| `BtlAim` | APUNTA: ^ ALTO  v BAJO | AIM: ^ HIGH  v LOW | P30 |
| `BtlShotHigh` | ¡TIRO ALTO! | HIGH SHOT! | P30 |
| `BtlShotLow` | ¡TIRO BAJO! | LOW SHOT! | P30 |
| `BtlHit` | ¡GOLPE! | HIT! | P30 |
| `BtlBlocked` | ¡BLOQUEADO! | BLOCKED! | P30 |
| `BtlBroken` | ¡GUARDIA ROTA! | GUARD BROKEN! | P30 |
| `BtlDodge` | ¡ESQUIVA! | DODGED! | P30 |
| `BtlWhiff` | EL TIRO SE PIERDE | THE SHOT MISSES | P30 |
| `BtlFoeTurn` | TURNO RIVAL | FOE'S TURN | P30 |
| `BtlFoeHit` | ¡TE DIO! | YOU'RE HIT! | P30 |
| `BtlFoeMiss` | ¡FALLÓ! | IT MISSED! | P30 |
| `BtlShieldHolds` | ¡EL ESCUDO AGUANTA! | SHIELD HOLDS! | P30 |
| `BtlRule` | A 3 GOLPES GANA | FIRST TO 3 HITS | P30 |
| `BtlVictory` | ¡VICTORIA! | VICTORY! | B10 |
| `BtlDefeat` | DERROTA | DEFEAT | B10 |
| `BtlDraw` | EMPATE: EL RIVAL HUYE | DRAW: THE FOE FLEES | P30 |
| `BtlRetreated` | HUISTE SIN DAÑO | RETREATED SAFELY | P30 |
| `BtlNoEscape` | ¡NO HAY ESCAPE! | NO ESCAPE! | N22 |
| `BtlItemShield` | OBJETO: ESCUDO | ITEM: SHIELD | P30 |
| `BtlHurt` | ¡HERIDO! -1 ÁNIMO | HURT! -1 MOOD | P30 |
| `BtlStreakLost` | ¡RACHA ROTA! VUELVES AL INICIO | STREAK LOST! BACK TO START | P30 |
| `BtlAreaClear` | ¡ZONA COMPLETADA! | AREA CLEARED! | P30 |
| `NoticeTooYoung` | CRECE PARA LUCHAR | GROW UP TO FIGHT | N22 |
| `NoticeAreaLocked` | ZONA BLOQUEADA | AREA LOCKED | N22 |
| `NoticeGone` | SE FUE... | IT'S GONE... | N22 |
| `HomeEncounter` | ENCUENTRO | ENCOUNTER | existing home label slot |
| `HomeBoss` | ¡JEFE! | BOSS! | existing home label slot |

Removed v1 strings: `NoticeNoVirux`, `NoticeHackerOnly`, `BattleAttack`, `BattleDefend`, `BattleFlee`, `BattleAim`, `BattleFoeStrikes`, `BattleFled`, `BattleCrit`, `BattleGlancing`, `BattleMiss`, `BattleAppeared`, `BattleWon`, `BattleLost`. Existing strings reused: `MenuTrain` (gate option), `Exp`, `NoticeSick`, `NoticeAsleep`, `HomeVirux`.

The existing i18n tests (table complete, widths fit, ASCII-only source, well-formed UTF-8) cover the new rows with no code change. Foe, boss and form names are not translated (Decision 19).

## Budgets

### Flash (target at most 40 KB added, expected about 33 KB)

| Item | Expected |
|---|---|
| `digivice_battle.c`, `digivice_expedition.c`, tables | 5 KB |
| i18n (about 85 new rows, two languages, minus about 14 removed) | 3.5 KB |
| Trail monsters (8 draw functions) | 5 KB |
| Bosses (8 draw functions) | 9 KB |
| Backgrounds (8 functions, scene cache, sky bands) | 6 KB |
| Shots, icons, effects, `digivice_draw_world.c` glue | 2 KB |
| App screens (expedition, map, hike, midway, gate, encounter, battle rewrite) | 8 KB |
| Service, notify sequences, API | 1.5 KB |
| v1 battle code removed | about -6 KB (not counted in the ceiling) |

Method: record the section sizes of the CI `firmware.elf` for v2.1 and for v3.1 and report the difference. Stop and re-scope (merge boss variants, cut foe frames) if it passes 40 KB.

### RAM

- Service stack (2 KB): `DigiviceState` is 116 bytes. The worst path (tick, then `digivice_changed`, then `digivice_publish`) holds `before`, `after` and `snapshot`: 348 bytes plus frames. The pace loop and the encounter roll use no buffers. The load chain holds the 56 byte V1 and the 84 byte V2 temporaries plus the 116 byte target. The device check logs `furi_thread_get_stack_space` after load and after a long offline tick and requires at least 512 bytes free.
- App heap: `DigiviceScene` about 420 bytes, allocated on entering the trail, encounter and boss screens, freed on leaving. `DigiviceApp` grows by about 80 bytes (battle state, hike state, auto-start memory). No frame buffer is cached.

## Error handling

- State migration as described: v1, v2 and v3 files load, anything else is a fresh egg. A failed migration load falls back to a fresh egg.
- Sanitize clamps every v3 field. A distance above the area length, an out-of-range area, or both encounter slots set are repaired on load, never trusted.
- A tick with `now < last_tick` (RTC set backwards) produces no paces and no expiry, as the existing elapsed clamp already does.
- A service call that arrives while the encounter already expired (a long fight) applies its result and clears nothing. A locked-area select returns false and the app shows "ZONA BLOQUEADA".
- Hike with the cap reached grants 0 and the screen says so. A Hike batch larger than the 24-beat session is clamped by the cap anyway.
- Every refusal is visible and localized: sick, asleep, too young, locked area, no escape.
- If the service is not running, the existing error screen shows, as before.

## Testing

- **Host rules tests (TDD, `digivice_rules_test.c` plus new `digivice_battle_test.c` and `digivice_expedition_test.c`), injected rng everywhere.**
  - Triangle: each of the 3 advantages, the 3 disadvantages, every Nulo pairing neutral, same-vs-same neutral, out-of-range inputs.
  - Hit chance: `100*P/(P+Q)` for several pairs, plus and minus 5 for attribute, both clamps (10 and 90), block halving, block minimum 5.
  - Form to attribute mapping for all 8 forms including Virubyte via `prev_form`; VIRUX type mapping.
  - Power: stage bases, strength cap at 24, injury x3/4, charge tiers.
  - Charge: tier in the zone, at the edges of "near", and far; zone width at strength 0, 8, 16, 24; timeout.
  - Guard: `guard_at` at frames 0, 4, 5, 9, 10 for both `g0`; sampling at the impact lag; blocked and unblocked cases.
  - Shot resolution: the four outcome cases with injected rng, the corruption miss, boss and foe turn symmetry.
  - 3-hit flow: player wins 3-0, 3-2, loses 2-3, ends at the exact shot that reaches 3, round-10 cap, draw, Shield cancelling the first foe hit once.
  - Retreat: allowed and refused for VIRUX, areas 1 to 4 trail, areas 1 to 4 boss, every streak area.
  - Results: EXP formulas for all 8 areas, boss repeat half, Shield drop at rng 9 and 10, guaranteed boss Shield, defeat penalties, injury timestamp, streak reset in areas 5 to 8 and no reset in 1 to 4, VIRUX v1 rewards.
  - `can_battle`: sick, asleep, too young, ok.
  - Paces: 1 per awake minute over a 60 s tick; carry across ticks; a 48 h clamped catch-up (about 1,920 paces); exact pace instants; no paces asleep, sick, Egg or at distance 0; Baby earns paces and can fight trail foes; a one-hour tick equals sixty one-minute ticks.
  - Hike: judge boundaries (149, 150, 151 ms), grant capped at 60 per window, window rollover at 3600 s, window shared with scans.
  - Scans: 3 paces per spawn deed in Hacker mode, none in Normal, cap 30 per window, non-spawn deeds grant nothing.
  - Encounters: spawn at roll 2 and no spawn at roll 3, guarantee at pace 50, `paces_since_fight` reset on spawn, no roll while one is pending or a VIRUX is active, pool index range, expiry at 30 min with no penalty, expired during offline catch-up leaves none pending, VIRUX and trail exclusivity.
  - Midway: availability at half length, each kind's effect, shortcut win and lose with clamps, done state, streak reset.
  - Boss and gate: ready at 0, gate refuses Rookie and Virubyte-from-Rookie and accepts Champion and Virubyte-from-Champion, clear marks the bit, advance to the lowest uncleared area, all-clear stays at area 8, locked areas, area switching resets progress.
  - `same_visible`: pace bookkeeping ignored, `distance == 0` flip, spawn and midway visible.
  - Persistence: `migrate_v2` field by field and defaults, the `rng` is nonzero, the v1 to v2 to v3 chain (a v1 fixture ends as a v3 state with the v1 values intact and lang and mode defaults), `last_tick` re-anchor is the service's job and is covered by the device check, sanitize clamps, `sizeof(DigiviceState) == 116` and the `offsetof` guard.
- **Host i18n test.** Unchanged code: all rows present, widths within budget, ASCII-only sources, UTF-8 well-formed. `run.sh` adds the new `.c` and `.h` files to the ASCII check.
- **`test/run.sh`** compiles the new modules with `-Wall -Wextra -Werror` next to `digivice_rules.c`, and the workflow keeps running it.
- **CI.** The fbt build stays green on `.github/workflows/digivice.yml` and publishes the `digivice-update-package`. The plan records the flash delta.
- **Device check** (user, after a USB flash with `scripts/selfupdate.py`):
  - The existing pet survives the update (form, EXP, mode, language), and keeps its progress. After the first launch no paces appear for the old time off, and a v1 save, if available, also loads.
  - The trail icon opens the Expedition screen in Normal and in Hacker mode, and there is no "SOLO EN MODO HACKER" anywhere.
  - Paces: leave the pet awake for about 10 minutes and see the distance fall by about 10. Sleep window (23:00 to 07:00) and sickness freeze it.
  - Hike: a session grants about the number of good taps, and the third session within an hour is capped at 60 total.
  - An encounter shows the LED and vibration, expires after 30 minutes without penalty, and the app auto-starts it on open. Declining with Back works. Retreat works in areas 1 to 4 and is refused in 5 to 8.
  - A full fight: charge, aim, the guard flicker, hit flash with shake, dodge, 3 hits to win, a drop, defeat with injury and bandage.
  - A midway event of each kind, and a boss in an early area (set up with a debug build or by editing the distance), including the gate screen as a Rookie.
  - Backgrounds: all 8, day and night, at 10 fps; draw time under 30 ms per frame from the log; no stutter on the map.
  - Hacker mode: scans add paces (3 per read deed, 30 per hour) and still spawn VIRUX.
  - `furi_thread_get_stack_space` of the service at least 512 bytes after a long offline tick.
  - Free flash after the flash is within the 40 KB ceiling.
  - The stock Flipper still works (menus, apps, the Hacker deeds).

## Out of scope for v3.1

- The 28-monster roster, the evolution tree and hidden evolution counters (v3.2).
- Calls, care mistakes rework, new feeding animations (v3.2).
- Cities, easter eggs and capture, NIDO and companions, ally rescue midway event, link battles (v3.3 and later).
- Chip damage on gated bosses, a defence minigame on the foe turn, more than one item, boss unlock rewards (wallpapers, branches, secrets) and credits.
- Per-area saved progress and a daily luck modifier.
- Translating the stock Flipper firmware.
