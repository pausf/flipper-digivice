#pragma once

// Every tuning number of the digivice lives here. Times are in seconds.

#define DIGIVICE_HUNGER_MAX 4
#define DIGIVICE_MOOD_MAX   4

#define DIGIVICE_HUNGER_DECAY_S (2UL * 3600UL)
#define DIGIVICE_MOOD_DECAY_S   (3UL * 3600UL)

#define DIGIVICE_SLEEP_START_HOUR 23
#define DIGIVICE_SLEEP_END_HOUR   7

#define DIGIVICE_ELAPSED_MAX_S (48UL * 3600UL)

#define DIGIVICE_CORRUPTION_MAX        100
#define DIGIVICE_CORRUPTION_PER_HOUR   5
#define DIGIVICE_CORRUPTION_FEED_DROP  5
#define DIGIVICE_CORRUPTION_TRAIN_DROP 10
#define DIGIVICE_CORRUPTION_LOSE_GAIN  10
#define DIGIVICE_CORRUPTION_GLITCH     50
#define DIGIVICE_CORRUPTION_MISS_PCT   25
#define DIGIVICE_CORRUPTION_RECOVERY_S (24UL * 3600UL)

#define DIGIVICE_HATCH_S      300UL
#define DIGIVICE_ROOKIE_EXP   50
#define DIGIVICE_CHAMPION_EXP 300

#define DIGIVICE_SPAWN_PCT   20
#define DIGIVICE_VIRUX_TTL_S (30UL * 60UL)

#define DIGIVICE_WIN_BASE_EXP 10
#define DIGIVICE_WIN_DATA     3

// Training EXP, granted in both modes.
#define DIGIVICE_TRAIN_EXP_OK   3
#define DIGIVICE_TRAIN_EXP_FAIL 1

// Care: poop, sickness and light. Mood decays DIGIVICE_MOOD_FAST_X times faster while sick or
// with at least DIGIVICE_POOP_MOOD_FAST piles (not stacked).
#define DIGIVICE_POOP_EVERY_S     (3UL * 3600UL)
#define DIGIVICE_POOP_MAX         4
#define DIGIVICE_POOP_MOOD_FAST   3
#define DIGIVICE_POOP_MISTAKE_S   (6UL * 3600UL)
#define DIGIVICE_MOOD_FAST_X      2
#define DIGIVICE_SICK_MISTAKES    2
#define DIGIVICE_STARVE_SICK_S    (24UL * 3600UL)
#define DIGIVICE_SICK_TWO_DOSES_S (12UL * 3600UL)
#define DIGIVICE_LIGHT_MISTAKE_S  3600UL

// Battle. Indexed by DigiviceStage: egg, baby, rookie, champion. Multipliers are x10.
#define DIGIVICE_STAGE_POWER       {0, 10, 20, 40}
#define DIGIVICE_VIRUX_POWER       {24, 24, 24, 46}
#define DIGIVICE_STRENGTH_CAP      24
#define DIGIVICE_CHARGE_X10        {6, 10, 15}
#define DIGIVICE_ATTR_PCT          5
#define DIGIVICE_HIT_MIN_PCT       10
#define DIGIVICE_HIT_MAX_PCT       90
#define DIGIVICE_BLOCK_MIN_PCT     5
#define DIGIVICE_HITS_TO_WIN       3
#define DIGIVICE_ROUND_CAP         10
#define DIGIVICE_ZONE_BASE_W       3
#define DIGIVICE_NEAR_SEGMENTS     2
#define DIGIVICE_CHARGE_FRAMES     30
#define DIGIVICE_AIM_FRAMES        30
#define DIGIVICE_GUARD_FLIP_FRAMES 5
#define DIGIVICE_IMPACT_LAG_FRAMES 5
#define DIGIVICE_TRAIL_EXP_BASE    6
#define DIGIVICE_TRAIL_EXP_AREA    2
#define DIGIVICE_BOSS_EXP_BASE     40
#define DIGIVICE_BOSS_EXP_AREA     20
#define DIGIVICE_DROP_PCT          10
#define DIGIVICE_INJURY_S          3600UL

// Projectile sizes in px; the Baby fires 2x2, the area 8 boss a 9x5 beam.
#define DIGIVICE_STAGE_SHOT      {2, 2, 3, 5}
#define DIGIVICE_SHOT_ROOKIE_FOE 3
#define DIGIVICE_SHOT_CHAMP_FOE  5
#define DIGIVICE_SHOT_BOSS       7
#define DIGIVICE_SHOT_BEAM       10

// Timing meter shared by training and battle.
#define DIGIVICE_METER_SEGMENTS 20
#define DIGIVICE_METER_ZONE     4

// Service timing.
#define DIGIVICE_TICK_S  60
#define DIGIVICE_FLUSH_S 30

// Profile screen: strength that fills the FUERZA bar.
#define DIGIVICE_STRENGTH_BAR_MAX 24

// App animation lengths in frames of the 100 ms clock.
#define DIGIVICE_NOTICE_FRAMES 15
#define DIGIVICE_CARE_FRAMES   30
#define DIGIVICE_FEED_FRAMES   50

// Expedition: 8 areas walked at 1 pace per awake minute.
#define DIGIVICE_AREA_COUNT          8
#define DIGIVICE_AREA_LENGTHS        {480, 600, 720, 840, 960, 1080, 1200, 1440}
#define DIGIVICE_ENCOUNTER_GUARANTEE 50
#define DIGIVICE_HIKE_CAP            60
#define DIGIVICE_SCAN_CAP            30
#define DIGIVICE_PACE_S              60
#define DIGIVICE_ENCOUNTER_PCT       3
#define DIGIVICE_ENCOUNTER_TTL_S     (30UL * 60UL)
#define DIGIVICE_STREAK_FIRST_AREA   4 // areas 5 to 8, 0-based
#define DIGIVICE_AREA_POWER          {10, 16, 24, 34, 46, 60, 76, 94}
#define DIGIVICE_CHAMP_FOE_X10       13
#define DIGIVICE_BOSS_X10            14

// Hike rhythm minigame and Hacker scans share one hourly bonus window.
#define DIGIVICE_HIKE_BEATS     24
#define DIGIVICE_HIKE_BEAT_MS   500
#define DIGIVICE_HIKE_GOOD_MS   150
#define DIGIVICE_SCAN_PACES     3
#define DIGIVICE_BONUS_WINDOW_S 3600UL

// Midway events.
#define DIGIVICE_CACHE_EXP              20
#define DIGIVICE_SPRING_CORRUPTION_DROP 10
#define DIGIVICE_SHORTCUT_WIN_PCT       50

// Cutscene length in frames of the 100 ms animation clock.
#define DIGIVICE_EVO_FRAMES 50

// Vigor: the strength hearts that protein fills (v3.2).
#define DIGIVICE_VIGOR_MAX     4
#define DIGIVICE_VIGOR_START   2
#define DIGIVICE_VIGOR_DECAY_S (4UL * 3600UL)

#define DIGIVICE_PROTEIN_STRENGTH 2

// Calls: awake seconds a hunger or vigor call waits before it becomes a care mistake.
#define DIGIVICE_CALL_WINDOW_S (10UL * 60UL)
