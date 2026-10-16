# Digivice v3 — status and handoff

Last updated: 2026-10-16

## Where things stand

| Item | State |
|---|---|
| v1 | On `main`. Pet, battle, evolution and HUD-cyber home. |
| v2.1 | Branch `digivice-v2.1`, not merged. Includes Spanish/English, the icon menu, poop/light/medicine/sickness, Normal and Hacker modes, and the v1→v2 save migration. Flashed and running on the device since 2026-10-14. |
| v3 | Designed, not specified or planned. |

## v3 inputs

- Research: [`research/2026-10-16-vpet-digivice-research.md`](research/2026-10-16-vpet-digivice-research.md). It studies how the Bandai V-Pet and Digivice devices work, with 34 sources and a design-implications section.
- Mockups (open in a browser, 128×64 1-bit, animated): [`mockups/v3/`](mockups/v3/)
  - `v3-bestiario.html`: 28 original monsters in 6 stages, plus the failure form, the evolution tree and the on-device dex.
  - `v3-expedicion.html`: area map, the "Hike" minigame, 8 areas with bosses, the boss gate, and seeded procedural backgrounds with day and night.
  - `v3-combate.html`: the Portador > Ruido > Cifrado > Portador triangle plus Nulo, an OK timing gauge, high/low aim, projectiles sized by stage, a hit flash, and a 3-hits rule.
  - `v3-cuidados.html`: food eaten in bites, the call alert with its 10-minute window, training, injury and sickness, and the evolution sequence.
  - `v3-ciudades.html`: city events for Barcelona, Madrid, Valencia, Sevilla and Bilbao, each with its landmarks and an exclusive monster.
  - `v3-easter-eggs.html`: hand-drawn homage partners (Adventure 1999 plus Terriermon). They appear about every 5 battles and can be caught. Caught partners live in the NIDO and walk with the pet.

## Decisions taken

- The full original lifecycle: 6 stages, about 28 monsters, Color-like timers.
- Hacker mode stays. Scans keep feeding the pet and spawning encounters.
- Normal mode gets Expedition: 1 pace per awake minute, plus the tap minigame capped at 60 paces per hour. Each pace has a 3% encounter chance, guaranteed by pace 50. Each area has a midway event and a boss at the end that needs Champion stage.
- Procedural backgrounds come from a seed, with parallax layers and day/night from the clock.
- Real-city events, with landmarks drawn as simplified original silhouettes.
- Easter eggs are **hand-drawn homages only**. No official Bandai sprites are copied, downloaded or traced. They sit behind the `DIGIVICE_EASTER_EGGS` build flag and stay out of any public or distributed build. A loader for user-provided sprites on the SD card has been offered but not decided.

## Proposed delivery order

1. Battle plus Expedition, with timed encounters in Normal mode. About 15–20% of the weekly plan limit.
2. The 28-monster roster, hidden evolution counters, calls and care mistakes, and the reworked animations.
3. Cities, easter eggs with capture, NIDO and the companion, plus procedural scenery polish.

Each delivery goes through spec, plan, subagent-driven implementation with reviews, a CI fbt build, a USB flash, and a device check.

## Device notes

- The Flipper "<name>" shows up on the Mac as `/dev/cu.usbmodemflip_<name>1`. Flash with
  `scripts/selfupdate.py -p <port> <update.fuf>`, using the CI artifact `digivice-update-package`.
- Backups live outside the repo in `~/flipper-backups/`: the previous Unleashed `unlshd-030e` SD data and `/int`, and the v1 save file.
