# V-Pet / Digivice Mechanics Study for an Original Flipper Zero Virtual Pet

Date: 2026-10-16
Purpose: document how Bandai's Digital Monster family of devices plays, so we can design an ORIGINAL pet for the Flipper Zero with the same depth. This document never proposes the use of any Digimon name, sprite or artwork. Franchise names appear only where a cited source needs them to identify a device or mechanic.

## How to read this document

- **[F]** marks a fact taken from a cited source. Source numbers like `[S4]` point to the Sources section at the end.
- **[I]** marks our inference, observation or design opinion. It is not sourced.
- Most detailed mechanics come from fan documentation: Humulos' manuals and guides, the With the Will LCD guides, and Wikimon. Bandai never published formulas, so treat even [F] numbers as community-verified, not official. Where sources disagree, both figures are given.
- Gaps are listed explicitly ("not found") instead of being filled in.

---

## 1. Device lineup and generations

### 1.1 Summary table

| Device (year) | Core loop | Input | Battles | Evolution | Connectivity |
|---|---|---|---|---|---|
| **Digital Monster V-Pet** Ver.1–5 (Jun 1997 JP) | Care pet: feed, train, clean, lights, heal [S1] | 3 buttons (A select, B confirm, C cancel) [S1][S4] | Only through a link, by touching metal contacts on two devices [S23] | Care mistakes, training, overfeed, sleep disturbance, battles and win ratio [S2] | Device-to-device contacts [S23] |
| **Digimon Pendulum** 1.0–5.0 + ZERO (Oct 1998 onward) [S6] | V-Pet care plus shake-to-train | Buttons plus an internal pendulum shake sensor [S6][S23] | Link battle: up to 3 shots each, best 2 of 3 [S6][S7] | 6 stages, adds Jogress (fusion) [S6] | Link: Battle and Jogress only [S6] |
| **Pendulum Progress** 1.0–3.0 (1.0 = Jul 2002) [S24] | Pendulum with a larger roster | Shake plus buttons | Adds a CPU opponent [S23] | Conditions tied to time-of-day training and "penalties" [S24] | Link |
| **Pendulum X** 1.0–3.0 (Apr–Nov 2003) [S22] | Pendulum plus an RPG quest | Shake | Quest on a 100-step board moved by die roll; HP bars [S22] | Strength, battles; Jogress [S22] | 3-prong link, item trade, Jogress [S22] |
| **Digivice** (Adventure, 1999 West) [S8] | Pedometer adventure across 7 fixed areas [S8] | 3 buttons plus pendulum step counter [S8] | Random battles while walking; mash Select to charge [S8] | Spend D-Power one stage per turn; shake on "Count" [S8] | VS link: "special" or V-Pet rules [S8] |
| **D-3** (Adventure 02) [S9] | Pedometer adventure | Buttons plus shake | Shake for Power; 4 sub-turns of shots that collide [S9] | Normal (costs D-Power) and Armor (egg items) [S9] | VS link; scan to a terminal on Ver.2 [S9] |
| **D-Power** (Tamers) [S10] | Pedometer adventure with map events | Buttons, shake, card-slash slot | Card slash for Battle and Evolve cards [S10] | Evolve Cards earned from events [S10] | VS link, scan [S10] |
| **Digivice iC / Data Link / Burst** (Savers) [S13][S14] | Pet care plus missions and a coliseum [S13] | 3 buttons, item sensor | Random battles; menu: Digivolve, Upgrade, Special, Help, Battle, Escape [S13] | Partner evolves only inside a battle, then reverts [S13] | C-Battle, item and point trade [S13] |
| **Digimon Accel** (2005, 4 versions) | Pet plus grip-pumping | Accel Grip, matrix sensor that scans chips and your hand [S11] | 10 HP bars; Single/Double/Triple/Hyper shots remove 1/2/3/4 bars; 4 areas of 10 opponents [S11] | "Blast Evolve" (30 DP) at the top stages doubles attack [S11] | Battle, trade, Jogress [S11] |
| **Digimon Twin** L/R (Mar 2007) [S12] | Pet sold in pairs | Buttons; mash B to train [S12] | Twin vs Twin, or vs iC/Burst [S12] | Needs 20+ battles; 40/60/80% win-ratio tiers; event links [search summary, S12] | "Event communication" gives food, items, eggs [S12] |
| **Vital Bracelet** (Mar 2021 JP) / **Vital Hero** (2022 US) / **BE** (Nov 2022) [S15][S16] | Fitness wearable: heart rate and steps feed the pet [S15] | Buttons, heart-rate sensor, pedometer, NFC [S15] | 5-shot HP race, BP vs hit chance [S17] | Time plus vital values plus battles plus training; 100-hour training limit on BE [S16][S17] | NFC, cards, apps (VB Lab shut down 13 Mar 2023) [S15] |
| **Digital Monster X** (Mar 2019) [S21] | Care plus levels and experience | Buttons | Quest map of about 30 areas; a luck roll ("XAI") then a timing gauge [S21] | Level, max level and HP; effort [S21] | Black + White link opens a hidden area [S21] |
| **Digital Monster COLOR** (Feb 2023 JP) [S18] | Color remake of the 1997 V-Pet; 6 stages [S18] | 3 buttons, USB-C [S18][S4] | Quest mode (first to 3 hits) plus link battles [S4] | Timers, mistakes, training, overfeed, sleep, win ratio [S3][S4] | Other COLOR-series devices only [S3] |
| **Digimon Pendulum COLOR** (Feb 2024 JP onward) [S19] | Color remake of Pendulum; 31–33 per version [S19] | Buttons | Quest battles with "cut-in" intros [S19] | Includes overnight evolutions after N trainings [S19] | COLOR family [S20] |
| **Digivice -25th COLOR EVOLUTION-** (Jul 29 2024 JP) [S20] | Pedometer story adventure | Buttons plus shake pedometer; a 5x step setting [S20] | Preset event at the end of each area; defeated enemies become "Friend" assists [S20] | Five "memory" bars filled by wins [S20] | Links to COLOR, Pendulum COLOR, D-3/D-Ark 25th [S20] |

Not researched in depth: the D-Ark toy and the Japanese-only Burst variants. Pendulum ZERO appears as the "Virus Busters" version in the Pendulum line [S6]. Sales context: about 14 million Digital Monster units by March 2004 (13 million in Japan), and more than 24 million by 2005 [S23].

### 1.2 Generational patterns [I]

1. **1997–2001: care pets.** The pet is the game. Battles happen only against a friend's device. Depth comes from hidden counters.
2. **1999–2004: pedometer adventures.** Progress becomes physical distance. Care shrinks to a medical menu. Battles become reflex minigames (mashing, shaking, card slashing), and bosses gate the areas.
3. **2003–2007: hybrids.** Care pets gain CPU opponents, HP bars, quest boards, items and DNA.
4. **2017–present: reissues and color remakes.** Single-player battles are now mandatory, the remakes are more forgiving (cold mode, backup, faster timers), and quest areas end in bosses.

---

## 2. Care mechanics (V-Pet 1997, Ver.20th, COLOR)

### 2.1 The eight-icon frame [F]

The original screen has 8 icons around its border [S1]:

| Icon | Function |
|---|---|
| Scale / status | Age and weight, hunger, strength, win ratio, DP gauge [S1] |
| Meat | Meat raises hunger. A protein pill raises strength and DP [S1] |
| Training | A successful session raises strength [S1] |
| Fight | Battle another device [S1] |
| Toilet | Clean up poop [S1] |
| Lights | Toggle the lights for sleep [S1] |
| Bandage | Medical treatment [S1] |
| Call | Lights up when the pet needs attention. It is not selectable [S1] |

On COLOR, A cycles the icons, B confirms and C cancels. Holding C opens settings [S4].

### 2.2 Stats

| Stat | Behaviour |
|---|---|
| **Hunger hearts** | 4 hearts [S3]. They drain over time at a rate that differs per species [S4]. Meat adds 1 heart and 1 G [S4] |
| **Strength hearts** | Protein adds 1 heart and 2 G [S4]. Training also fills them. On Ver.20th, mashing 13 or more presses gives 2 hearts [S5]. Each heart adds 4 Power on Ver.20th, up to +16 [S5] |
| **Effort** | 1 heart per 4 training sessions, successful or not [S3][S4]. Ver.20th counts it as +0.25 per session [S5]. Wikimon's Ver.20th page counts only successful rounds [search summary, S31] |
| **DP / Energy** | Needed to battle (at least 1) [S4]. Restored by 8 hours of sleep [S1][S4]. 4 protein = 1 DP on COLOR [S4]. On Ver.20th the cap is 14 at Stage III and +14 per stage [S5] |
| **Weight (G)** | Food +1, protein +2, training −2, battle −4 on the original [S1][S6]. COLOR: training −1 [S4]. At 99 G on Ver.20th, the Power bonus is lost [S5] |
| **Age** | 1 year = 24 hours [S1][S6] |
| **Win ratio** | Counted across all stages on COLOR. It is the only counter that survives evolution [S3]. Ver.20th counts only the last 15 battles [S5] |
| **Power** | A species base (4–238 on Ver.20th) plus a heart bonus [S5]. COLOR bonus per stage: +5/+8/+15/+25 for full strength, and the same again for a traited egg [S4] |

### 2.3 The call and care mistakes

- **What causes a call:** hunger empty, strength empty, or it is lights-out time [S1][S4]. On the Pendulum, also sickness or dying [S7].
- **Window length (it differs by version):** 20 minutes on the 1997 V-Pet and Pendulum [S1][S2][S7]. 10 minutes for hunger or strength on Ver.20th and COLOR [S4][S5]. 1 hour for the sleep call on COLOR [S4]. 20 or 30 minutes for lights on the originals [S1][S7].
- **Counting:** if the call light goes out on its own, that is one care mistake [S3][S4]. When hunger and strength empty at the same time, the 1997 V-Pet counts one mistake [S2] and COLOR counts two [S3]. An empty meter can call again later [S4].
- **What does NOT count:** poop, injury, overfeeding and waking the pet (COLOR, Ver.20th) [S3][S5]. On Ver.1, up to 4 poops have no effect [S2].
- **Reset:** the count returns to 0 on each evolution [S2][S4].

### 2.4 Overfeeding and sleep disturbance

- **Overfeeding:** keep feeding until the pet refuses food; that refusal counts as 1 overfeed. A hunger heart must drain before the next one can count [S2][S3]. On COLOR, 10 extra meat gives "one extra hunger loss cycle" [S4].
- **Sleep disturbance:** waking a sleeping pet by feeding, training or battling counts 1 [S2][S3]. Setting the clock to daytime does not count on Ver.1 [S2]. On COLOR, the pet falls back asleep after 10 minutes [S4].

### 2.5 Sleep schedule

- Bedtime is set per species [I, inferred from the per-species sleep calls in [S1] and the guides]. Everyone wakes at 8:00 on COLOR [S4] and at 7:00 on Ver.20th [S5].
- Ver.20th has 3-hour naps, and its evolution timer pauses during sleep [S5]. On COLOR, sleep time counts toward the timer [S3]. On Digital Monster X it does not pause [S21].
- On the Pendulum, missing lights-out means the pet wakes 1 hour later [S7].

### 2.6 Poop, sickness, injury and the bandage

- **Poop interval:** about every 2 h on the 1997 V-Pet [S1]. About every 3 h on the Pendulum [S7]. COLOR: every 3 min at Stage I, every 60 min at Stage II, then every 120 min [S4].
- **Sickness or injury from poop:** 8 uncleaned piles cause sickness on the V-Pet and Pendulum [S1][S7] and injury on COLOR [S4]. On Ver.20th, 4 piles cause injury [S5].
- **Injury from battle (COLOR):** 20% after a win and 10% after a loss. Each protein overdose adds +10%, capped at 80% [S4].
- **Bandage:** the pet may refuse treatment, so you repeat the action [S1][S7]. Some species need several doses [S4][S5].

### 2.7 Death [F]

| Cause (COLOR [S3][S4]) | Ver.20th [S5] |
|---|---|
| Injured 15 times in one form | 20 injuries in one form |
| Injured for 6 consecutive hours | Injured for 6 hours |
| Hunger or strength empty for 12 consecutive hours | Same |
| Stage IV/V timer ends with ≥5 mistakes and requirements unmet | 20 care mistakes in one form |
| Stage VI after 48 h in form with ≥5 mistakes | 5 mistakes after 48 h awake in the stage |

- **Rebirth:** press A+B on the grave to get a new egg [S4].
- **Traited egg:** Stage V+ deaths have a 30% chance of a traited egg, which gives a power bonus to the next pet (COLOR) [S3][S4]. On Ver.20th, you get one if the pet survived 48 h past its last evolution [S5].
- **Cold mode / backup:** frozen pets don't age, call or lose hearts [S4].

---

## 3. Evolution system

### 3.1 Stages and timers

| Stage | Common names | COLOR timer to the next stage [S4] | Vital Bracelet timer [S15] |
|---|---|---|---|
| Egg | Digitama | 8 s | — |
| I | Baby I / Fresh | 10 min | 1 h |
| II | Baby II / In-Training | 12 h | 3 h |
| III | Child / Rookie | 24 h | 16 h |
| IV | Adult / Champion | 36 h | 24 h |
| V | Perfect / Ultimate | 48 h | 24 h |
| VI | Ultimate / Mega | (end; VI+ via Jogress) | 24 h, or 3 h via Jogress |

On Ver.1 (1997), Baby I → Baby II takes about 60 minutes [S2]. The 1997 V-Pet has 5 stages and its Pendulum successor has 6 [S1][S6]. One reviewer says the final stage now takes "as little as two days" versus six on the original [S27].

### 3.2 Branching criteria (worked example: 1997 Ver.1) [F, S2]

- **Baby II → Child:** 0–3 calls gives the "good" Child; 4 or more calls gives the "rough" Child.
- **Child → Adult, good branch:** calls 0–3 and training 32+ give the strongest Adult. Calls 0–3 with low training give a dark Adult.
- **Child → Adult, rough branch:** combinations of calls ≥4, a training band (5–15, 8–31, 16+), overfeeding (0–3 or 3+/4+) and sleep disturbances (0–4, 0–6, 0–8, 9+) select between four other Adults.
- **Fallback:** a slug-like Adult catches every pet that matches none of the conditions above.
- **Adult → Perfect:** 15+ battles with ≥80% wins. Each Perfect has a pool of Adults that can reach it.

**Roster sizes:** 1997 Ver.1 has 1/1/2/7/3 monsters per stage, 14 in total [S2]. COLOR Ver.1–5 have 18–20 monsters per version [S3][S18]. Pendulum COLOR has 31–33 [S19]. Vital Bracelet Dim cards hold up to 17, and later cards up to 23 [S15][S16]. Digital Monster X has 30 per version [S21].

**COLOR rule refinements [S3]:**
- **Win ratio** gives a probability at Stage V/VI: ≥80% → 100%, 60–79% → 50%, 40–59% → 25%, <40% → no evolution.
- **Battles** must happen in the current stage.
- **Alternatives:** when a requirement lists several bullets, meeting any one is enough.
- **Time of day:** some evolutions need day (06:00–17:59) or night [search summary, S3].
- **Unlisted stats:** a stat that is not listed in a requirement does not matter.

### 3.3 Fusion and special evolutions

- **Jogress (fusion):** two compatible pets link and one or both evolve [S6]. It needs full DP [S7]. On COLOR, two Stage VI pets must win or fight a connection battle and both evolve to VI+ [S3]. On Pendulum X, a failed attempt zeroes Strength [S22].
- **Armor evolution (D-3):** uses egg items instead of D-Power, and the egg can be lost after battle [S9].
- **Blast Evolve (Accel):** keeps the same form but changes the look, doubles attack and makes the Hyper Hit a one-hit kill. Costs 30 DP [S11].
- **Battle-only evolution (iC):** the partner evolves only inside a fight, needs matching DNA, and reverts afterward [S13].
- **Overnight evolutions after N trainings (Pendulum COLOR)** [S19].
- **Mode Change:** a special form shown only during the attack animation [S18][S19].
- **Hidden routes:** some unlock by linking two versions or beating a hidden boss (Digital Monster X) [S21], or by clearing certain adventure stages (Vital Bracelet) [S15][S16].

---

## 4. Battle systems

### 4.1 V-Pet and COLOR battles

- **Hit formula (COLOR) [F, S4]:** `hit% = (P_player × 100) / (P_player + P_enemy) + attr`, where attr is +5 for an advantage, −5 for a disadvantage and 0 otherwise. On Ver.20th, an advantage instead acts like "+32 Power" [S5].
- **Attribute triangle [F, S4][S5]:** Vaccine beats Virus, Virus beats Data, Data beats Vaccine. Free is neutral.
- **Quest mode:** the first side to land 3 hits wins [S4].
- **Connection battle:** the first 3 hits always land and the 4th decides the winner [S4]. [I] This makes every link fight look long and dramatic while one roll actually decides it.
- **Pendulum link:** up to 3 shots each, best 2 of 3 [S6][S7].
- **Ver.20th:** single battles fill a meter by mashing A. Tag battles stop a gauge and then mash. Attack tiers are Weak, Strong, Double Weak, Double Strong and Critical [S5].
- **Requirements:** Stage III or higher and DP ≥1 [S4]. Baby forms cannot battle [S1][S6].

**What the player sees in a V-Pet or COLOR battle [I, typical sequence assembled from the sources above and widely shared device footage; not documented step-by-step in any fetched source]:**

1. The player picks the Fight icon. On COLOR, quest areas open with an intro card; Pendulum COLOR uses a "cut-in" [S19].
2. "VS"-style splash, then both monsters face each other: the player on the right, the enemy on the left (mirrored).
3. **Round loop:**
   - The attacker shows its open-mouth or attack frame and spits a projectile. The projectile is a small glyph, and its size grows with stage or strength tier.
   - The projectile slides horizontally across the screen in a few pixel steps.
   - On a hit, the target flashes or blinks and a small burst appears. On a miss, the shot passes over or under the target or vanishes.
   - The defender's turn follows.
4. After the deciding hit, the loser flops into a sad or injured pose. The winner shows its happy frame bouncing, with a win or lose screen.
5. Afterward the injury icon or bandage may appear, along with the weight and DP deductions.

### 4.2 Pendulum shaking and training shots [F]

- **Pendulum:** shaking sets a counter that becomes 5 shots. Each shot is a single or a "Super Hit" (double). Zero Super Hits means a failed session. 5 Super Hits make a "Mega Hit", which fills the Strength meter [S6][S7].
- **Pendulum X and Accel:** shot scores are Single 0, Double 1, Triple 2, Super/Hyper 4. 5 or more points succeeds. Five of a kind gives a Mega, Giga or Tera Hit [S11][S22].
- **Pendulum X quest:** HP is 6 bars per side and shots remove 1–3 bars. On 1.0, a Super Hit takes all 6 [S22].

### 4.3 Pedometer Digivices

- **Digivice (1999):**
  - The player moves through 7 fixed areas [S8].
  - Random enemies attack while walking. The player mashes Select to charge, and enough presses mean the partner attacks; otherwise the enemy does [S8].
  - When the distance counter hits 0, the area boss appears [S8].
  - Child-stage partners cannot damage bosses, which forces evolution [S8]. Evolution spends D-Power and asks the player to shake on "Count" [S8].
  - Slot and card minigames add or subtract distance [S8].
- **D-3:**
  - The player shakes to build Power at the start of each turn [S9].
  - Each turn has 4 sub-turns. In each one, a Digimon either fires or doesn't, with a probability set by Power [S9].
  - Shots collide in the middle: equal sizes cancel, and the larger destroys the smaller and deals damage equal to its size. Shot size equals Attack, with occasional variation [S9].
  - Ver.1 has a halfway boss in each of its 3 locations [S9].
  - Ver.2 areas end in one of 4 events: a boss, a tower to destroy, a recruitable ally, or a village that restores D-Power [S9].
- **D-Power:**
  - Each level has 4 end-of-location events whose placement is randomised [S10].
  - Leaving a location loses some or all of its progress [S10].
  - The boss can be locked until a story event happens [S10].
- **-25th-:**
  - 7 maps, with 4–13 areas per stage [S20].
  - Each area has a preset event at 0 steps: a cutscene or a battle [S20].
  - Defeated enemies become "Friend" assists [S20].
  - Evolution unlocks after 5 "memory" bars fill through wins [S20].
  - Reviews describe three battle input styles: mashing, shaking and follow-the-arrow [search summary of S20 reviews].

### 4.4 Accel and Twin

- **Accel:** 4 areas of 10 opponents, playable in any order. The player has 10 HP bars, shots remove 1–4 bars, and each defeated enemy drops an item [S11].
- **Twin:** battles only through Connect [S12]. Evolution needs 20+ battles and a win ratio tier [search summary of S12].

### 4.5 Vital Bracelet [F, S17]

- **Requirements:** Stage III or higher.
- **Round structure:** each side fires up to 5 shots. The side that drops to 0 HP first loses, or the side with lower HP after 5 rounds.
- **Stats:** BP raises hit chance and attribute shifts it. AP is damage, with ±5% from mood. Super Hits come from the monster's activity type.
- **Stat growth:** HP, AP and BP come from 20–30 s exercise sessions, rated Good or Great.
- **Stakes:** vital values change with the stage matchup (e.g. +200 for a III win, −160 for a III loss).
- **Adventure:** 12 stages on BE cards and 15 on the original Dim cards. Each stage means walking a set distance and then fighting a boss [S17]. Clearing stages unlocks evolutions [S15][S16].
- **Death:** 4 injuries in a day, or 24 h without wearing the device [S17].

### 4.6 COLOR and X quest modes

- **COLOR quest:** each area is several battles ending in a boss. Area 1 is 3 battles [S3]. The boss unlocks the next area [S4]. Ver.1/2 lock the player inside the area until they win the boss or lose; Ver.3–5 allow exiting between battles [S4]. Each round counts as a battle for win ratio [S3]. Clears unlock wallpapers [S4].
- **Digital Monster X:** about 30 areas fought as streaks ("win in a row, or restart the area"). Higher areas have rest stops for feeding, and a daily luck roll changes enemy strength [S21]. Experience thresholds run Lv2 = 50 … Lv10 = 5000 [S21].

---

## 5. Encounter design without connectivity

**Facts:**
- **Distance-gated bosses:** the boss sits at distance 0 in every pedometer device [S8][S9][S10][S20].
- **Random fights:** they happen "while walking" [S8][S9][S10]. No fetched source gives an encounter rate or per-area distances.
- **Mid-area structure:** the D-3 has a halfway boss [S9], and the D-Power randomises event placement [S10].
- **Distance minigames:** slot and card games change distance on the Digivice and D-3 [S8][S9].
- **Streak areas:** X and COLOR (Ver.1/2) make the player win an area's fights consecutively [S4][S21].
- **Difficulty knobs:** stage gates, such as Child partners being unable to hurt bosses [S8], and a daily luck modifier on X [S21].
- **Item drops:** Accel drops an item per kill [S11].

**Inference [I]:**
- Difficulty scales mainly through enemy stage and power by area index, plus the requirement to evolve before the boss.
- With the counters roughly measured in thousands of steps, fan reports put a typical area at about a day of normal walking, but we found no numeric source. Treat the pacing as an open question.

---

## 6. UI and visual design (for a pixel artist)

**Sourced facts:**
- The original LCD is monochrome with an 8-icon border [S1].
- Community sprite extractions treat each creature as **16 px native 1-bit sprites with 11 poses** [S25]:
  - 0–1: idle or walk A/B
  - 2–3: sleep A/B
  - 4: angry, refuse or attack
  - 5: happy
  - 6: sad
  - 7: eat with mouth closed, or chew
  - 8: eat with mouth open, or neutral
  - 9: tired, sick or old
  - 10: exhausted
- "Refuse" is drawn as a left/right **mirror flip on alternating frames** (a head shake) [S25].
- The Vital Bracelet walks its monster when the user walks and has exercise animations [S28].
- COLOR "Mode Change" happens during the attack animation [S18].

**Unverified:** a 32×16 dot-matrix screen and 16×16 sprites for the 1997 LCD are widely repeated, but no fetched source confirmed them. The 16 px figure above is from a community extraction [S25].

### 6.1 Screen-by-screen FEEL [I: observational description for recreation, not sourced]

| Screen | Layout | Motion |
|---|---|---|
| **Home** | Icons in a row above and below the play field. The pet wanders left/right in 1–2 px steps | 2-frame idle at about 1–2 fps. It turns around at the edges by mirroring. Poop piles stack in a corner with a 2-frame "stink wave" |
| **Clock** | Large HH:MM, which is the "screen saver" | Static or blinking colon |
| **Status** | Paged screens: age/weight, hunger hearts, strength hearts, effort hearts, DP, win % | Hearts are fixed outlines that fill in solid |
| **Eating** | Food sprite (about 8×8) sits beside the mouth | Mouth open, then closed, repeated 3 times. The food loses a chunk each time (full → ¾ → ½ → crumbs → gone). Then a happy frame. Refusal is the mirror head-shake |
| **Training** | The pet on one side, a target or wall on the other | Charge pose, then a projectile flies, then the target breaks or wobbles. Then a happy or sad pose |
| **Battle** | Two sprites face each other, the enemy mirrored | Shot glyph travels about 4 px/frame; the hit flash is an inverted or blinking sprite plus a burst star. The deciding hit shakes the screen |
| **Lights off** | Screen goes fully dark (inverted) with the sleeping pet and "Z z" bubbles | 2-frame sleep breathing; the Z rises 1 px per frame |
| **Call** | The call icon blinks, with a beep | Pet shows the angry or sad frame |
| **Sick or injured** | Skull or bandage glyph near the pet | Tired frame |
| **Evolution** | Center stage, old and new forms | The two forms alternate faster and faster under strobe flashes, with a full-screen invert, then the new form appears on a happy frame |
| **Death** | Grave or ghost sprite | Static, waits for a button combo [S4] |

---

## 7. Monster design language [I, pattern analysis; no names reused]

| Stage | Silhouette rules | Size on device | Typical count per version |
|---|---|---|---|
| Egg | Oval with a pattern band; the pattern hints at the line | Small | 1 (or several eggs on COLOR) |
| I (Fresh) | Round blob, eyes only, no limbs; bounces | About ½ sprite height | 1 |
| II (In-Training) | Blob with one feature (ears, tail, horn nub, leaves) | About ⅔ height | 1 |
| III (Rookie) | Small whole creature: stubby legs, big head, simple mouth; often a small reptile, mammal or plant-sprite | Fills most of the sprite | 2 (the 1997 V-Pet) [S2] |
| IV (Champion) | Adds a power signifier: horns, wings, armor plates, a weapon, flames; often heavier and more angular | Full sprite, broader | 5–7 [S2] |
| V (Ultimate) | Hybridised: cybernetic parts, extra limbs or mythic motifs | Full sprite; detail is dense | 3 [S2] |
| VI (Mega) | Armored humanoid, knight or giant beast; the silhouette breaks the square frame (capes, spikes) | Full sprite | 3 or more [S4] |

**Attribute silhouettes, as a tendency rather than a rule:**
- **Vaccine:** clean, symmetrical, round; heroic armor.
- **Data:** creature-like or natural; often aquatic or plant.
- **Virus:** jagged, dark fills, demonic or slimy shapes.

**The failure form:** every tree has a deliberately pathetic "failure" form that absorbs pets matching no requirement [S2]. Fans remember it fondly. That last point is our own observation, not sourced.

---

## 8. Why fans love it, and what they complain about

**Fun or addictive:**
- **Hidden counters:** they create mystery and replay ("what do I get if…"). [I, supported by the guide culture in S3]
- **Low-pressure nostalgia:** reissues "respect your time" and feel "more like coaching than parenting" [S26].
- **Pacing:** the final stage takes about 2 days instead of 6 [S27].
- **Social battles:** being able to fight without a friend's device now matters, and linking still adds value [S26][S27].
- **Fairness:** food and medicine are unlimited, so outcomes depend on the player's attention, not on luck [S29, fan comment].
- **Vital Bracelet loop:** exercise feeds the monster, and the walking and exercise animations charm players; filling the dex is a strong pull [S28].
- **Choice per egg:** the egg sets which paths are possible, and a second pet doubles the options [S26].

**Complaints:**
- **Investment:** specific forms need precise tracking and are "still an investment". Overfeeding easily sends the pet down the wrong branch [S26].
- **Punishment:** constant punishment makes missed care feel unfair. Players want a mode where neglect does not kill [S29].
- **Exploits:** players farm weak opponents to pad their win ratio [S29].
- **Completionism:** it needs "military precision" in deliberately neglecting the pet just enough [S30, search snippet].
- **Daily caps:** exercise rewards are capped at once per day; app shutdowns lost pets; the hardware felt cheap (Vital Hero) [S28].
- **Single-device play:** r/digimonvpet users praise being able to run one device and still unlock everything [S34].

---

## 9. Design implications for our Flipper digivice

Everything below is **[I]**, our original design proposal.

### 9.1 Platform constraints

- **Hardware:** 128×64 1-bit LCD; d-pad plus OK and Back; RGB LED; vibration; buzzer; ~240 KB flash budget.
- **Sprite memory:** a 32×32 1-bit frame is 128 B. 28 monsters × 11 poses × 128 B ≈ **39 KB**. 16×16 sprites drawn at 2× cost 32 B per frame, about **10 KB**. Either fits.
- **Recommendation:** author at 24×24 or 32×32. This gives roughly 4× the 1997 detail but keeps the chunky feel. Draw the effects (projectiles, bursts, hearts, Z's, food bites) procedurally.
- **Offline time:** the app is not always in the foreground. Store a timestamp and **simulate the elapsed time on launch** using the RTC: hearts drain, poop accrues, timers run. Calls that expire while the app is closed should count at most 1 mistake per category per absence (lenient catch-up). Freeze when the battery is low, like COLOR's cold mode [S4].

### 9.2 Attribute triangle (original names, signal theme)

The theme fits the Flipper's radio identity. Each attribute beats the next in a cycle; Null is neutral.

| Attribute | Beats | Visual cues |
|---|---|---|
| **Carrier** | Noise | Smooth curves, sine motifs, symmetrical |
| **Noise** | Cipher | Jagged, static speckle dither, asymmetry |
| **Cipher** | Carrier | Blocky, grid, keyhole and circuit motifs |
| **Null** | — | Hollow outlines, minimal |

Battle formula, kept from COLOR because it is simple and readable:
`hit = clamp(100·P/(P+Q) + 6·adv, 10, 90)`, where adv is +1 for an advantage, −1 for a disadvantage and 0 for neutral.

### 9.3 Evolution tree: 26 monsters, 7 stages, 3 egg lines sharing a trunk

The codenames are placeholders that describe the silhouette.

| Stage (timer) | Monsters |
|---|---|
| Egg (30 s) | EGG-STRIPE (line A), EGG-DOT (line B) |
| I · Seed (1 h) | A: DROPLET (blob, eyes). B: SPARKLET (blob with an antenna nub) |
| II · Sprout (12 h) | A: EARBLOB (blob with ears). B: TAILBLOB (blob with a tail) |
| III · Rookie (24 h) | Good care (≤2 mistakes): A: PUP-HORNLET (Carrier); B: GECKO-CHIP (Cipher). Rough (≥3): A: IMP-STATIC (Noise); B: SLIME-BYTE (Null) |
| IV · Champion (36 h) | **From PUP-HORNLET:** RAM-CHARGER (Carrier: train ≥24, mistakes ≤1); WOLF-ECHO (Carrier: train 12–23); BAT-GLITCH (Noise: mistakes ≥2) |
| | **From GECKO-CHIP:** BEETLE-LOCK (Cipher: train ≥24); OWL-SCANNER (Cipher: ≥3 sleep disturbances, night owl) |
| | **From IMP-STATIC:** HOUND-FUZZ (Noise: train ≥16); TOAD-HISS (Noise: overfeed ≥3) |
| | **From SLIME-BYTE:** GOLEM-SCRAP (Null: train ≥16) |
| | **Failure form, from any Rookie when nothing matches:** PUDDLE-JUNK (droopy, ears flat) |
| V · Ultimate (48 h; ≥12 battles at ≥60%) | KNIGHT-ANTENNA (Carrier; from RAM or WOLF). WYVERN-STATIC (Noise; from BAT or HOUND). TITAN-VAULT (Cipher; from BEETLE or OWL). MIRE-KING (Noise; from TOAD or PUDDLE-JUNK, the redemption path). COLOSSUS-SCRAP (Null; from GOLEM) |
| VI · Apex (end; ≤1 mistake and ≥80%) | SERAPH-WAVE (Carrier). DRAGON-NOISE (Noise). ORACLE-CIPHER (Cipher). VOID-SOVEREIGN (Null; from COLOSSUS or MIRE-KING after defeating the final boss) |
| Fusion (VI+) | PRISM-REGENT = Carrier Apex + Cipher Apex, made through an IR link battle between two Flippers, or by defeating a hidden boss solo |
| Secret | GHOST-BEACON: unlocked by clearing area 6 with a Rookie (a "speedrun" trophy) |

**Total:** 2 eggs (not counted) + 2 + 2 + 4 + 9 + 5 + 4 + 2 = **28 monsters**. This is within the COLOR 18–20 range plus extras [S3].

Rules carried from the originals:
- Counters reset on evolution, while win ratio and lifetime battle records carry over [S3].
- Any one bullet of a requirement suffices [S3].
- Unlisted stats don't matter [S3].
- A failure form exists with a redemption route [S2].

### 9.4 Care-mistake rules

| Event | Window | Mistake? | Feedback |
|---|---|---|---|
| Hunger empty | 15 min | Yes, 1 | LED amber blink, 2 short buzzes |
| Strength empty | 15 min | Yes, 1 (counted separately, as on COLOR [S3]) | Same |
| Bedtime (per species, e.g. 20:00–22:00) | 60 min to dim the lights | Yes, 1 | LED blue blink |
| Poop | 4 piles cause sickness | No, but sickness lasting 6 h kills | Stink animation |
| Overfeed | After refusal; one more needs a heart to drain | No (an evolution counter) | Head-shake |
| Wake during sleep | — | No (sleep-disturbance counter) | Grumpy frame |
| Injury | Battle: 15% after a win, 30% after a loss | No; 10 injuries in a form or 6 h untreated kills | Bandage glyph |
| Death | Hearts empty 12 h; ≥5 mistakes when a stage IV/V timer ends | — | Grave screen, then a new egg |

Add a **"Gentle mode"** setting where neglect injures but never kills. This answers the most common complaint [S29].

### 9.5 Battle flow on the d-pad (about 20 s per fight)

1. **Intro:** "VS" banner wipes in; the two sprites slide in from the edges; the LED flashes the enemy attribute's colour.
2. **Charge (2 s):** a gauge sweeps. Press OK to stop it in the sweet zone. The zone is wider with more strength hearts and narrower when overweight (echoing the X gauge [S21]). The result sets the shot tier: Small, Big or Double.
3. **Aim:** press Up or Down to pick a high or low shot. The enemy picks a random guard. A matched guard halves the damage (echoing Ver.1 training's high/low [S4]).
4. **Exchange:** roll the hit formula. Fire the projectile; it travels in 6 frames at 50 ms each.
   - **Hit:** invert the target for 2 frames, show a 5×5 burst star, shake the screen ±1 px, buzz, give a 40 ms vibration and flash the LED red.
   - **Miss:** the shot sails off the edge, with a "whiff" sound.
   - The enemy then fires back automatically.
5. **Repeat** until 3 hits (quest mode, like COLOR [S4]) or HP 0.
6. **Result:**
   - **Victory:** the winner does a 2-frame jump loop, a "WIN" banner drops in, a rising arpeggio plays and the LED goes green.
   - **Defeat:** the sad frame plays, the sprite sinks 2 px, a falling tone plays, the LED goes red, and there is an injury roll.
7. **Bookkeeping:** −1 DP and −2 weight. The battle is recorded.
8. **Link battle (later):** two Flippers trade stats over IR. Run COLOR's "first 3 hits land, the 4th decides" rule [S4] so both screens animate the same pre-rolled script, seeded from both devices.

### 9.6 Expedition mode (no step counter)

- **Concept:** the pet travels while time passes. The player picks an area, and **Trail** progress accrues from real time while the pet is awake: 1 pace per minute, simulated offline.
- **Accelerating:** the player can speed it up with a **"Hike" minigame** (rhythm-tap OK to the beat; each good tap = 1 pace). It is capped at 60 paces per hour, so tapping helps but doesn't trivialise the area.
- **Encounters:** each pace rolls an encounter at 3%, so roughly one every 30 minutes. The roll is guaranteed by pace 50 since the last fight. A notification (LED and vibration) waits up to 30 minutes; skipping it costs nothing except the encounter.
- **Area structure:** 3–6 trail fights, a midway event (one of: rest spring that heals and fills hearts, item cache, slot-style "shortcut" gamble that changes distance ±20%, or ally rescue), then the boss at 0, as on the D-3 and D-Power [S9][S10].
- **Streaks:** areas 1–4 allow retreat. Areas 5–8 are streak areas where losing resets the area's distance, following X [S21].
- **Scaling:** enemy power = area base × stage factor. Bosses require at least Champion; a Rookie does chip damage only, echoing [S8].

| # | Original area theme | Pace length | Boss (silhouette) | Unlock |
|---|---|---|---|---|
| 1 | Static Meadow (TV-snow grass) | 120 | Bloated crackle-toad | Wallpaper |
| 2 | Copper Grove (wire trees) | 180 | Coiled cable serpent | Champion branch: OWL-SCANNER |
| 3 | Antenna Ridge | 240 | Mast-backed ram | Item: shield |
| 4 | Frozen Cache (ice blocks) | 300 | Crystalline vault beetle | Ultimate branch: TITAN-VAULT |
| 5 | Magnet Dunes (streak) | 360 | Iron-filings sand wurm | Fusion hint |
| 6 | Relay Tower (streak) | 420 | Strobing sentinel | Secret GHOST-BEACON (if cleared with a Rookie) |
| 7 | Deep Buffer (sunken data sea) | 480 | Overflow leviathan | Apex condition |
| 8 | Null Core | 600 | Silhouette made of negative space (an inverted sprite of your own pet) | VOID-SOVEREIGN, credits |

**Pacing:** areas 1–8 total about 2,700 paces, which is about 45 hours awake. That matches the COLOR 36–48 h stage timers [S4], so a full run spans one lifetime.

### 9.7 Animation specifications

- **Feeding:** a 10×8 food sprite with 4 masks (100/66/33/0). Mouth open, then closed, 3 times at 300 ms, removing a mask each time. A crumb particle falls 3 px. End with the happy frame and a heart pop. Protein is a capsule that "clinks".
- **Training:**
  - Choice of 3 drills (one per attribute flavour): stop the gauge; mash OK for 3 s; high/low read.
  - The target (wall, can or bell) cracks in 3 damage states.
  - Success shows sparkles. Failure shows a slumped pose.
- **Battle:** as in 9.5. Projectile glyphs grow by stage (3×3 at Rookie, 5×5 at Champion, 7×7 at Ultimate, a 9×5 beam at Apex). Each attribute has a projectile style: Carrier = ring wave, Noise = jagged bolt, Cipher = square packet.
- **Hit flash:** XOR the target's sprite rectangle for 2 frames, flash the LED red, vibrate for 40 ms.
- **Evolution:**
  - About 4 s: old and new silhouettes alternate at 400, 250, 150, 80 and 40 ms.
  - Between swaps, the full screen inverts. Then a white flash and a radial line burst.
  - The LED cycles R-G-B, an ascending 5-note jingle plays, the new form shows its happy frame, and a name card follows.
- **Lights off:** invert the screen and dim the backlight. Two-frame breathing every 1 s; a "z" rises from the head in 4 positions. Poop stays visible.
- **Death:** a grave sprite with a slowly blinking cursor. The new egg rocks left and right (2 frames) before hatching.
- **Idle personality:** each species gets one quirk from a small library (hop, spin by mirror flip, sit, look-around). Driving it procedurally keeps flash usage low.

### 9.8 Top five recommendations

1. **Keep the hidden counters** (mistakes, training, overfeed, sleep disturbance, battles and win ratio). Reset them on evolution and keep a failure form plus a redemption path. This is the core of the "depth" [S2][S3].
2. **Use time-and-tap expeditions with boss-gated areas** to replace the step counter, with random trail fights and stage-gated bosses [S8][S9][S20].
3. **Use COLOR's hit formula with an original 3+1 signal triangle,** plus a d-pad charge-and-aim layer so fights feel physical [S4].
4. **Simulate elapsed time offline with lenient catch-up and add a Gentle mode.** This fixes the most cited frustration [S29][S26].
5. **Use an 11-pose sprite grammar plus procedural effects** (bites, projectiles, XOR hit flash, strobe evolution, plus the LED, buzzer and vibration). This gets the authentic feel inside about 40 KB [S25].

---

## Sources

- [S1] With the Will LCD — Digimon Virtual Pet intro: https://lcd.withthewill.net/VpetIntro
- [S2] Wikimon — Digital Monster Ver. 1: https://wikimon.net/Digital_Monster_Ver._1
- [S3] Humulos — Digital Monster COLOR evolution guide and FAQ: https://humulos.com/digimon/dmc
- [S4] Humulos — Digital Monster COLOR manual: https://humulos.com/digimon/dmc/manual
- [S5] Humulos — Digital Monster Ver.20th manual: https://humulos.com/digimon/dm20/manual
- [S6] Wikimon — Digimon Pendulum: https://wikimon.net/Digimon_Pendulum
- [S7] With the Will LCD — Pendulum intro: https://lcd.withthewill.net/PenIntro
- [S8] With the Will LCD — Digivice (US) intro: https://lcd.withthewill.net/DigiviceUsIntro
- [S9] With the Will LCD — D-3 (US) intro: https://lcd.withthewill.net/DThreeUsIntro
- [S10] With the Will LCD — D-Power (EU) intro: https://lcd.withthewill.net/DPowerEuIntro
- [S11] With the Will LCD — Digimon Accel intro: https://lcd.withthewill.net/AccelIntro
- [S12] Wikimon — Digimon Twin: https://wikimon.net/Digimon_Twin (evolution tiers from the search summary of https://wikimon.net/Digimon_Twin_L)
- [S13] With the Will LCD — Data Link (Digivice iC) intro: https://lcd.withthewill.net/DataLinkIntro
- [S14] Wikimon — Digivice Burst (search summary only): https://wikimon.net/Digivice_Burst
- [S15] Wikimon — Vital Bracelet Digital Monster: https://wikimon.net/Vital_Bracelet_Digital_Monster
- [S16] Wikimon — Vital Bracelet BE: https://wikimon.net/Vital_Bracelet_BE
- [S17] Humulos — Vital Bracelet BE manual: https://humulos.com/digimon/vbbe/manual
- [S18] Wikimon — Digital Monster COLOR: https://wikimon.net/Digital_Monster_COLOR and https://wikimon.net/Digital_Monster_COLOR_Ver.1
- [S19] Wikimon — Digimon Pendulum COLOR: https://wikimon.net/Digimon_Pendulum_COLOR
- [S20] Wikimon — Digivice -25th COLOR EVOLUTION-: https://wikimon.net/Digivice_-25th_Color_Evolution- (review claims on battle styles via https://gamerbraves.com/digimon-adventure-digivice-25th-color-evolution-dx-set-taichi-yagami-review-a-nostalgic-digital-adventure-reborn, search summary)
- [S21] Wikimon — Digital Monster X: https://wikimon.net/Digital_Monster_X
- [S22] Wikimon — Digimon Pendulum X: https://wikimon.net/Digimon_Pendulum_X
- [S23] Wikipedia — Digital Monster: https://en.wikipedia.org/wiki/Digital_Monster
- [S24] Wikimon — Pendulum Progress 1.0 (search summary): https://wikimon.net/Pendulum_Progress_1.0
- [S25] tuipet (community V-Pet clone; pose taxonomy and 16 px native sprites): https://pypi.org/project/tuipet/
- [S26] Siliconera — 20th anniversary V-Pet impressions: https://www.siliconera.com/the-20-anniversary-digimon-virtual-pet-feel-quite-nostalgic/
- [S27] VPet Paradise — Is the 20th anniversary V-Pet worth buying: https://vpetparadise.com/2019/07/07/is-the-20th-anniversary-digimon-virtual-pet-worth-buying-plus-release-date-price-and-more/
- [S28] Irreverently (Substack) — The Digimon fitness bracelet: https://irreverently.substack.com/p/the-digimon-fitness-bracelet-killed
- [S29] itch.io fan comments on a V-Pet-style game (search summary): https://aludeku.itch.io/digital-partner-digimon/comments?after=60
- [S30] GamerBraves — Digital Monster COLOR review (403 on fetch; search snippet only): https://www.gamerbraves.com/?p=148842
- [S31] Wikimon — Digital Monster Ver.20th (search summary): https://www.wikimon.net/Digital_Monster_Ver.20th
- [S32] Wikimon — Digital Monster Ver. 2 (search summary): https://wikimon.net/Digital_Monster_Ver._2
- [S33] Toy People — Digital Monster COLOR announcement ("Single Player Battle Mode"): https://www.toy-people.com/en/?p=74237
- [S34] GummySearch — r/digimonvpet overview: https://gummysearch.com/r/digimonvpet/

### Known gaps

- Encounter rates, per-area step distances and on-screen battle frame counts for the 1999-era Digivices were not found in any source.
- The 1997 LCD resolution (32×16 is often claimed) is unverified.
- Vital Bracelet hit formula: Humulos points to an external breakdown that was not fetched.
- D-Ark and Digivice iC/Burst battle internals were not researched in depth.
- Direct Reddit threads were not reachable; fan sentiment comes from reviews and secondary aggregators.
