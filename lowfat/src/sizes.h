#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// Every region for objects of different sizes is split into heap/stack(/global)
// parts We use the configuration proposed in the latest paper: 32GiB, thereof
// 12GiB heap, 16GiB global, 4GiB stack

// 0x100000000 for 4GB, 0x800000000 for 32GB
#define REGION_SIZE 0x800000000ULL
// 32U for 4GB, 35U for 32GB
// binary log of REGION_SIZE, if region size is changed, this must be adapted as
// well
#define REGION_SIZE_LOG 35U

// TODO the original implementation has awfully odd numbers, find out why
#define HEAP_REGION_SIZE 0x300000000ULL
#define GLOBAL_REGION_SIZE 0x400000000ULL
#define STACK_REGION_SIZE 0x100000000ULL

#define HEAP_REGION_OFFSET 0U
#define GLOBAL_REGION_OFFSET (HEAP_REGION_OFFSET + HEAP_REGION_SIZE)
#define STACK_REGION_OFFSET (GLOBAL_REGION_OFFSET + GLOBAL_REGION_SIZE)

// min permitted low-fat object size
#define MIN_PERMITTED_LF_SIZE 16U
// binary log of min permitted low-fat object size
#define MIN_PERMITTED_LF_SIZE_LOG 4U
// max permitted low-fat object size, anything above will be allocated with a
// fall back glibc allocator 0x40000000ULL = 1GB
#define MAX_PERMITTED_LF_SIZE 0x40000000ULL

#define MAX_PERMITTED_LF_STACK_ALLOC_SIZE 0x40000000ULL

#define STACK_SIZE 0x40000000ULL

// number of low-fat regions, i.e. how many permitted allocation sizes
// there are 27 powers of 2 from 16B to 1GB
#define NUM_REGIONS 27U

#define BASE_STACK_REGION_NUM 28U

static const uint64_t STACK_SIZES[65] = {
    0,        // index 0
    0,        // index 1
    0,        // index 2
    0,        // index 3
    0,        // index 4
    0,        // index 5
    0,        // index 6
    0,        // index 7
    0,        // index 8
    0,        // index 9
    0,        // index 10
    0,        // index 11
    0,        // index 12
    0,        // index 13
    0,        // index 14
    0,        // index 15
    0,        // index 16
    0,        // index 17
    0,        // index 18
    0,        // index 19
    0,        // index 20
    0,        // index 21
    0,        // index 22
    0,        // index 23
    0,        // index 24
    0,        // index 25
    0,        // index 26
    0,        // index 27
    0,        // index 28
    0,        // index 29
    0,        // index 30
    0,        // index 31
    0,        // index 32
    0,        // index 33
    0,        // index 34
    0,        // index 35
    0,        // index 36
    0,        // index 37
    0,        // index 38
    33554432, // index 39
    16777216, // index 40
    8388608,  // index 41
    4194304,  // index 42
    2097152,  // index 43
    1048576,  // index 44
    524288,   // index 45
    262144,   // index 46
    131072,   // index 47
    65536,    // index 48
    32768,    // index 49
    16384,    // index 50
    8192,     // index 51
    4096,     // index 52
    2048,     // index 53
    1024,     // index 54
    512,      // index 55
    256,      // index 56
    128,      // index 57
    64,       // index 58
    32,       // index 59
    16,       // index 60
    16,       // index 61
    16,       // index 62
    16,       // index 63
    16,       // index 64
};

static const uint64_t STACK_MASKS[65] = {
    0,                     // index 0
    0,                     // index 1
    0,                     // index 2
    0,                     // index 3
    0,                     // index 4
    0,                     // index 5
    0,                     // index 6
    0,                     // index 7
    0,                     // index 8
    0,                     // index 9
    0,                     // index 10
    0,                     // index 11
    0,                     // index 12
    0,                     // index 13
    0,                     // index 14
    0,                     // index 15
    0,                     // index 16
    0,                     // index 17
    0,                     // index 18
    0,                     // index 19
    0,                     // index 20
    0,                     // index 21
    0,                     // index 22
    0,                     // index 23
    0,                     // index 24
    0,                     // index 25
    0,                     // index 26
    0,                     // index 27
    0,                     // index 28
    0,                     // index 29
    0,                     // index 30
    0,                     // index 31
    0,                     // index 32
    0,                     // index 33
    0,                     // index 34
    0,                     // index 35
    0,                     // index 36
    0,                     // index 37
    0,                     // index 38
    0xFFFFFFFFFE000000ull, // index 39, size 33554432
    0xFFFFFFFFFF000000ull, // index 40, size 16777216
    0xFFFFFFFFFF800000ull, // index 41, size 8388608
    0xFFFFFFFFFFC00000ull, // index 42, size 4194304
    0xFFFFFFFFFFE00000ull, // index 43, size 2097152
    0xFFFFFFFFFFF00000ull, // index 44, size 1048576
    0xFFFFFFFFFFF80000ull, // index 45, size 524288
    0xFFFFFFFFFFFC0000ull, // index 46, size 262144
    0xFFFFFFFFFFFE0000ull, // index 47, size 131072
    0xFFFFFFFFFFFF0000ull, // index 48, size 65536
    0xFFFFFFFFFFFF8000ull, // index 49, size 32768
    0xFFFFFFFFFFFFC000ull, // index 50, size 16384
    0xFFFFFFFFFFFFE000ull, // index 51, size 8192
    0xFFFFFFFFFFFFF000ull, // index 52, size 4096
    0xFFFFFFFFFFFFF800ull, // index 53, size 2048
    0xFFFFFFFFFFFFFC00ull, // index 54, size 1024
    0xFFFFFFFFFFFFFE00ull, // index 55, size 512
    0xFFFFFFFFFFFFFF00ull, // index 56, size 256
    0xFFFFFFFFFFFFFF80ull, // index 57, size 128
    0xFFFFFFFFFFFFFFC0ull, // index 58, size 64
    0xFFFFFFFFFFFFFFE0ull, // index 59, size 32
    0xFFFFFFFFFFFFFFF0ull, // index 60, size 16
    0xFFFFFFFFFFFFFFF0ull, // index 61, size 16
    0xFFFFFFFFFFFFFFF0ull, // index 62, size 16
    0xFFFFFFFFFFFFFFF0ull, // index 63, size 16
    0xFFFFFFFFFFFFFFF0ull, // index 64, size 16
};

static const int64_t STACK_OFFSETS[65] = {
    0,             // index 0
    0,             // index 1
    0,             // index 2
    0,             // index 3
    0,             // index 4
    0,             // index 5
    0,             // index 6
    0,             // index 7
    0,             // index 8
    0,             // index 9
    0,             // index 10
    0,             // index 11
    0,             // index 12
    0,             // index 13
    0,             // index 14
    0,             // index 15
    0,             // index 16
    0,             // index 17
    0,             // index 18
    0,             // index 19
    0,             // index 20
    0,             // index 21
    0,             // index 22
    0,             // index 23
    0,             // index 24
    0,             // index 25
    0,             // index 26
    0,             // index 27
    0,             // index 28
    0,             // index 29
    0,             // index 30
    0,             // index 31
    0,             // index 32
    0,             // index 33
    0,             // index 34
    0,             // index 35
    0,             // index 36
    0,             // index 37
    0,             // index 38
    -206158430208, // index 39, size 33554432
    -240518168576, // index 40, size 16777216
    -274877906944, // index 41, size 8388608
    -309237645312, // index 42, size 4194304
    -343597383680, // index 43, size 2097152
    -377957122048, // index 44, size 1048576
    -412316860416, // index 45, size 524288
    -446676598784, // index 46, size 262144
    -481036337152, // index 47, size 131072
    -515396075520, // index 48, size 65536
    -549755813888, // index 49, size 32768
    -584115552256, // index 50, size 16384
    -618475290624, // index 51, size 8192
    -652835028992, // index 52, size 4096
    -687194767360, // index 53, size 2048
    -721554505728, // index 54, size 1024
    -755914244096, // index 55, size 512
    -790273982464, // index 56, size 256
    -824633720832, // index 57, size 128
    -858993459200, // index 58, size 64
    -893353197568, // index 59, size 32
    -927712935936, // index 60, size 16
    -927712935936, // index 61, size 16
    -927712935936, // index 62, size 16
    -927712935936, // index 63, size 16
    -927712935936, // index 64, size 16
};
