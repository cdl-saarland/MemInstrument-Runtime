#pragma once

/** GENERAL **/
#define REGION_SIZE 0x800000000ULL // 0x100000000 for 4GB, 0x800000000 for 32GB
// binary log of REGION_SIZE, if region size is changed, this must be adapted as well
#define REGION_SIZE_LOG 35U // 32U for 4GB, 35U for 32GB

// min permitted low-fat object size
#define MIN_PERMITTED_LF_SIZE 16U
// binary log of min permitted low-fat object size
#define MIN_PERMITTED_LF_SIZE_LOG 4U
// max permitted low-fat object size, anything above will be allocated with a fall back glibc allocator
// 0x40000000ULL = 1GB
#define MAX_PERMITTED_LF_SIZE 0x40000000ULL

// number of low-fat regions, i.e. how many permitted allocation sizes
// there are 27 powers of 2 from 16B to 1GB
#define NUM_REGIONS 27U
/******************/

/** STATISTICS **/
#define MIRT_STATISTICS 1
#define MIRT_STATS_COUNTER_DEFS "statistic_counters.def"
#define MIRT_STATS_FILE "mi_stats.txt"
/****************/
