#pragma once

#define REGION_SIZE 0x100000000 // 0x100000000 for 4GB

// TODO maybe replace this with the size of sizes[]
#define NUM_REGIONS 4

// faster size index computation if sizes only contains powers of 2
#define POW_2_SIZES