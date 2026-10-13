# Digivice for Flipper Zero — Design

Date: 2026-10-13
Status: Draft, pending user review

## Goal

Turn the Flipper Zero into a digivice-style virtual pet **without removing or
changing any stock Flipper functionality**. The pet lives on the home screen,
grows from how the Flipper is actually used (NFC, Sub-GHz, IR, BadUSB…), needs
care, and fights "VIRUX" enemies that appear while scanning.

Non-goals for v1: battles between two Flippers, an Ultimate evolution stage,
licensed Digimon characters or art.

## Constraints

- Fork of the official firmware (`flipperdevices/flipperzero-firmware`). The
  diff against upstream must stay small so each official release rebases cleanly.
- Bluetooth, the mobile app, qFlipper, RPC, storage and updater stay untouched.
- Display is 128×64, 1-bit. "Colour" comes from the RGB LED; feel comes from
  vibration and the buzzer.
- Installing the official firmware (e.g. from the mobile app) removes the
  digivice. Custom builds are installed with qFlipper "Install from file" or
  `./fbt flash_usb_full`.

## Visual direction

**HUD cyber** (chosen from mockups): perspective grid floor, focus brackets
around the pet and enemies, segmented HP bars, scan-line sweep, glitch effects
on hits and on corruption, rotating dashed rings during evolution.

## Architecture

The fork touches four areas:

### 1. Dolphin deed hook (`applications/services/dolphin/`)

- Add a second `FuriPubSub* deed_pubsub` to `Dolphin` (`dolphin_i.h`) and expose
  it via `dolphin_get_deed_pubsub()` in `dolphin.h`.
- In `dolphin_process_event`, on a deed event, publish the `DolphinDeed` value
  next to the existing `DolphinPubsubEventUpdate` publish (`dolphin.c:228`).
- No deed caller changes. Dolphin XP/levels behave exactly as before.

### 2. `digivice` service (`applications/services/digivice/`)

- `apptype=SERVICE`, added to `basic_services`. Its `order` is after dolphin and
  notification, and it `requires` both. Stack ≈1.5K. It creates `RECORD_DIGIVICE`.
- Same skeleton as dolphin: `FuriEventLoop` + message queue + timers.
- The deed pubsub callback puts the deed on the service queue with **timeout 0**.
  If the queue is full, that deed is dropped, so dolphin and apps never block.
- A 60 s periodic timer applies time-based rules.
- Persistence: `saved_struct` at `INT_PATH(".digivice.state")` with a version
  and magic. Flush happens 30 s after the last change, like dolphin.
- Owns a `FuriPubSub` of its own ("state changed") for the desktop view and the
  battle app.
- Drives the LED, vibration and buzzer through `RECORD_NOTIFICATION` with custom
  `NotificationSequence`s.

### 3. Desktop home view (`applications/services/desktop/`)

- In `desktop.c:286`, the view stack gets the digivice view instead of the
  animation-manager dolphin view (also in `locked_view_stack`).
- The view draws the HUD-cyber home: pet, hunger and mood, corruption glitches,
  and a VIRUX alert.
- The input callback consumes only **short Right** and returns `false` for
  everything else. All existing desktop shortcuts keep working: OK = menu,
  Up = lock menu, Down = archive, Left/long keys = favourites, long Back = power.
- **Short Right** opens the digivice menu (replaces the Passport shortcut).
- `animation_manager` stays allocated but hidden, so the dolphin level-up flow
  (`dolphin_upgrade_level`) keeps working. Accepted loss: the animated
  "SD card bad / no database" notices. The HUD shows a small SD warning icon
  instead.

### 4. Digivice app (`applications/main/digivice/`)

- Internal `APP` launched by the loader from the digivice menu. It is a normal
  app, so the desktop pauses while it runs.
- Scenes: menu (Feed, Train, Stats, Battle), training minigame, battle,
  evolution cutscene.
- Talks to the service through `RECORD_DIGIVICE` (read state, submit actions).

### Pure rules module

`digivice_rules.c/.h` holds all game logic as pure C (no furi includes): decay,
corruption, evolution, deed mapping, damage, type chart, spawn roll (RNG passed
in). The service and the app both call it. All tuning numbers live in
`digivice_config.h`.

### Assets

Forms are drawn procedurally with 1-bit canvas primitives (no bitmaps, no assets
folder). Each form is a drawing function with poses: idle (2-frame breathing plus
a blink), attack, and hit (the caller blinks the sprite). Corruption uses the
VIRUBYTE form plus a glitch overlay. No SD card required.

### Cleanup

Remove `LOADER_AUTOSTART = "digivice"` from `fbt_options.py` and the early
placeholder app; it is replaced by section 4.

## Game rules (v1)

### Persisted state

form, exp, data[4] (per type), hunger 0–4, mood 0–4, corruption 0–100,
strength, wins, losses, pending VIRUX (type + expiry timestamp),
last_tick_timestamp, pre-corruption form, clean-since timestamp.

### Data types

| Type    | Deeds                                         |
|---------|-----------------------------------------------|
| NFC     | all `DolphinAppNfc`, `DolphinAppRfid`, `DolphinAppIbutton` deeds |
| SUBGHZ  | all `DolphinAppSubGhz` deeds                  |
| IR      | all `DolphinAppIr` deeds                      |
| USB     | `BadUsbPlayScript`, `U2fAuthorized`, `GpioUartBridge` |
| (none)  | `PluginStart`, `PluginGameStart`, `PluginGameWin`: EXP only |

Every deed grants EXP equal to `dolphin_deed_get_weight(deed)` and +1 data of
its type.

### Care

- Hunger −1 every 2 h; mood −1 every 3 h.
- Sleep window 23:00–07:00 (RTC): no decay.
- Feed: hunger +1 (max 4).
- Train: a few-second timing minigame. Success gives strength +1 and mood +1;
  both success and failure cost hunger −1.
- Elapsed time per tick is clamped to **[0, 48 h]**. This covers clock changes
  and long power-off periods.

### Corruption

- +5 per hour while hunger == 0 or mood == 0.
- Feeding −5, successful training −10 (floor 0).
- ≥ 50: glitch visuals and a 25 % chance per attack to miss.
- 100: transforms into the corrupt form (saving the previous form).
- Back to the previous form after corruption stays at 0 for 24 h.

### Evolution (7 forms)

| Stage    | Form                                            | Condition                    |
|----------|-------------------------------------------------|------------------------------|
| Egg      | BYTEEGG                                         | start / reset                |
| Baby     | BITLING                                         | 1 h after hatch start        |
| Rookie   | BYTEMON                                         | exp ≥ 50                     |
| Champion | TAGDRAKE (NFC), WAVEDRAKE (SUBGHZ), BEAMDRAKE (IR), PLUGDRAKE (USB) | exp ≥ 300, highest data type; ties resolve in that order |
| Corrupt  | VIRUBYTE                                        | corruption reaches 100 (Rookie or later) |

Form names are placeholders and can be renamed freely. Evolution plays the
cutscene the next time the app or home view is shown.

### VIRUX

- Spawn roll: 20 % on any read/scan deed (`NfcRead`, `RfidRead`, `IbuttonRead`,
  `SubGhzReceiverInfo`, `SubGhzFrequencyAnalyzer`).
- Only when the form is Rookie or later. At most 1 pending.
- The VIRUX takes the deed's type and expires after 30 min.
- On spawn: red LED blink + short vibration; the home HUD shows an alert.

## Battle

- Turn-based. Player actions:
  - **Attack**: a timing bar on a segmented HUD meter. Inside the zone = ×1.5,
    outside = ×0.5.
  - **Defend**: the next incoming hit is halved.
  - **Flee**: 50 %. On success the VIRUX is gone, with no penalty.
- Type chart, ×1.5 for the attacker: NFC > USB > IR > SUBGHZ > NFC. The player's
  type is its Champion type; Rookie and Corrupt are neutral.
- Damage = base(stage) + strength, times the timing multiplier, times the type
  multiplier.
- VIRUX HP and attack scale with the player's stage.
- Win: exp ×2 of the base reward, +3 data of the VIRUX type.
  Lose: corruption +10. Either way the VIRUX is removed.
- Feedback: LED cyan while charging, red on hit, vibration on hits, buzzer on
  win and evolution, glitch effect on hits.

## Error handling

- Missing, corrupt or version-mismatched state file: start a fresh egg and log
  it. Never crash.
- Deed queue full: drop the deed, never block.
- RTC moved backwards: elapsed time clamps to 0.
- Service record missing in the app (it should not happen): show an error
  screen and exit.

## Testing

- `digivice_rules` is pure C, covered by a host-side assert test compiled with
  the system `cc` (`applications/services/digivice/test/`), so it runs on the
  Mac in seconds without building or flashing firmware. The tests cover:
  - decay
  - sleep window
  - clamp
  - corruption up/down/transform/recovery
  - each evolution path and tie-break
  - deed→type mapping
  - damage and type chart
  - spawn eligibility
- The user verifies UI, service integration, and that the stock features (BT
  pairing, mobile app connection, NFC/Sub-GHz apps, lock screen, shortcuts)
  still work on the device.

## Out of scope (later versions)

- Flipper-vs-Flipper battles (IR/BLE).
- Ultimate stage and bigger evolution tree.
- Animated SD-card notices on the home screen.
