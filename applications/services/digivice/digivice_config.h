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

#define DIGIVICE_HATCH_S      3600UL
#define DIGIVICE_ROOKIE_EXP   50
#define DIGIVICE_CHAMPION_EXP 300

#define DIGIVICE_SPAWN_PCT   20
#define DIGIVICE_VIRUX_TTL_S (30UL * 60UL)

#define DIGIVICE_WIN_BASE_EXP 10
#define DIGIVICE_WIN_DATA     3
#define DIGIVICE_FLEE_PCT     50

// Multipliers are stored x10 so the rules stay in integer math.
#define DIGIVICE_TIMING_HIT_X10  15
#define DIGIVICE_TIMING_MISS_X10 5
#define DIGIVICE_TYPE_ADV_X10    15

// Indexed by DigiviceStage: egg, baby, rookie, champion.
#define DIGIVICE_BASE_DAMAGE {1, 2, 4, 7}
#define DIGIVICE_PLAYER_HP   {10, 14, 24, 40}
#define DIGIVICE_VIRUX_HP    {10, 14, 22, 38}
#define DIGIVICE_VIRUX_ATK   {2, 3, 4, 7}

// Timing meter shared by training and battle.
#define DIGIVICE_METER_SEGMENTS 20
#define DIGIVICE_METER_ZONE     4

// Service timing.
#define DIGIVICE_TICK_S  60
#define DIGIVICE_FLUSH_S 30

// Cutscene length in frames of the 100 ms animation clock.
#define DIGIVICE_EVO_FRAMES 50
