#pragma once

/** GENERAL **/
#define REGION_SIZE 0x100000000ULL // 0x100000000 for 4GB
#define REGION_SIZE_LOG 32U // binary log of REGION_SIZE, if region size is changed, this must be adapted as well
/*************/

/** OPTIMIZATION MODES **/
// NOTE: Only one of these should be used at a time

//#define FAST_BASE // multiply with (pre calculated) inverse of size
#define POW_2_SIZES // faster size index computation if sizes only contains powers of 2

#ifdef FAST_BASE
#define FAST_BASE_NO_PAD_LIMIT 2097152 // 2MB in paper
#endif

#ifdef POW_2_SIZES
#define NUM_REGIONS 27U
#else
#define NUM_REGIONS 529U
#endif
/******************/

/** STATISTICS **/
#define STATISTICS 1
#define STATS_COUNTER_DEFS "statistic_counters.def"
#define STATS_FILE "MI_STATS_FILE"
/****************/