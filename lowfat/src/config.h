#pragma once

#define REGION_SIZE 0x100000000ULL // 0x100000000 for 4GB

// binary log of REGION_SIZE, if region size is changed, this must be adapted correspondingly
#define REGION_SIZE_LOG 32U

// TODO maybe replace this with the size of sizes[]
#define NUM_REGIONS 529U

// multiply with (stored) inverse of size
#define FAST_BASE

// faster size index computation if sizes only contains powers of 2
//#define POW_2_SIZES