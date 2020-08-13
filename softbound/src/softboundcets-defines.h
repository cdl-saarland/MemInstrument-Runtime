//=== runtime/softboundcets.h - Prototypes for internal functions --*- C -*===//
// Copyright (c) 2016 Santosh Nagarakatte. All rights reserved.

// Developed by: Santosh Nagarakatte, Rutgers University
//               http://www.cs.rutgers.edu/~santosh.nagarakatte/softbound/

// The  SoftBoundCETS project had contributions from:
// Santosh Nagarakatte, Rutgers University,
// Milo M K Martin, University of Pennsylvania,
// Steve Zdancewic, University of Pennsylvania,
// Jianzhou Zhao, University of Pennsylvania

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal with the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

//   1. Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimers.

//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimers in the
//      documentation and/or other materials provided with the distribution.

//   3. Neither the names of its developers nor the names of its
//      contributors may be used to endorse or promote products
//      derived from this software without specific prior written
//      permission.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// WITH THE SOFTWARE.
//===---------------------------------------------------------------------===//

#ifndef __SOFTBOUNDCETS_H__
#define __SOFTBOUNDCETS_H__

#include <assert.h>
#include <stddef.h>
#include <unistd.h>

// Ensure that we work on a 64bit system
static_assert(__WORDSIZE == 64, "This library is coded for 64bit systems only");

//===----------------------------------------------------------------------===//
//                       Configuration options
//===----------------------------------------------------------------------===//

// Configure the run-time to ensure...
//  * spatial safety only:  __SOFTBOUNDCETS_SPATIAL
//  * temporal safety only: __SOFTBOUNDCETS_TEMPORAL
//  * both:                 __SOFTBOUNDCETS_SPATIAL_TEMPORAL

#ifndef __SOFTBOUNDCETS_SPATIAL
#define __SOFTBOUNDCETS_SPATIAL 0
#endif

#ifndef __SOFTBOUNDCETS_TEMPORAL
#define __SOFTBOUNDCETS_TEMPORAL 0
#endif

#ifndef __SOFTBOUNDCETS_SPATIAL_TEMPORAL
#define __SOFTBOUNDCETS_SPATIAL_TEMPORAL 0
#endif

// Enforce that a configuration is given
static_assert(__SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_TEMPORAL ||
                  __SOFTBOUNDCETS_SPATIAL_TEMPORAL,
              "Invalid configuration. Please decide to configure the run-time "
              "to either full safety (__SOFTBOUNDCETS_SPATIAL_TEMPORAL), "
              "temporal safety only (__SOFTBOUNDCETS_TEMPORAL) or spatial "
              "safety only (__SOFTBOUNDCETS_SPATIAL)");

// Enforce that exactly one configuration is given
static_assert((__SOFTBOUNDCETS_SPATIAL ^ __SOFTBOUNDCETS_TEMPORAL ^
               __SOFTBOUNDCETS_SPATIAL_TEMPORAL) &&
                  !(__SOFTBOUNDCETS_SPATIAL && __SOFTBOUNDCETS_TEMPORAL &&
                    __SOFTBOUNDCETS_SPATIAL_TEMPORAL),
              "Invalid configuration. Please use only exactly one of "
              "__SOFTBOUNDCETS_SPATIAL, __SOFTBOUNDCETS_TEMPORAL and "
              "__SOFTBOUNDCETS_SPATIAL_TEMPORAL.");

// Option to not report errors at all [testing only]
#ifndef NOERRORS
#define NOERRORS 0
#endif

// Option to not report errors if a bound value is null [testing only]
#ifndef NOERRORMISSINGBOUNDS
#define NOERRORMISSINGBOUNDS 0
#endif

// Option for printing statistics about dynamically executed checks [testing
// only]
#ifndef ENABLE_RT_STATS
#define ENABLE_RT_STATS 0
#endif

// TODO add description of this option
#ifndef __SOFTBOUNDCETS_PREALLOCATE_TRIE
#define __SOFTBOUNDCETS_PREALLOCATE_TRIE 0
#endif

// TODO add description of this option
#ifndef __SOFTBOUNDCETS_FREE_MAP
#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL || __SOFTBOUNDCETS_TEMPORAL
#define __SOFTBOUNDCETS_FREE_MAP 1
#endif
#endif

//===----------------------------------------------------------------------===//
//                              Shorthands
//===----------------------------------------------------------------------===//

#define __WEAK__ __attribute__((__weak__))

#define __WEAK_INLINE __attribute__((__weak__, __always_inline__))

#define __METADATA_INLINE __attribute__((__weak__, __always_inline__))

#define __NO_INLINE __attribute__((__noinline__))

#if defined(__APPLE__)
#define SOFTBOUNDCETS_MMAP_FLAGS (MAP_ANON | MAP_NORESERVE | MAP_PRIVATE)
#else
#define SOFTBOUNDCETS_MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE)
#endif

//===----------------------------------------------------------------------===//
//                            Data structures
//===----------------------------------------------------------------------===//

/* Trie represented by the following by a structure with four fields
 * if both __SOFTBOUNDCETS_SPATIAL and __SOFTBOUNDCETS_TEMPORAL are
 * specified. It has key and lock with size_t
 */
typedef struct {

#if __SOFTBOUNDCETS_SPATIAL
    void *base;
    void *bound;

#define __SOFTBOUNDCETS_METADATA_NUM_FIELDS 2
#define __BASE_INDEX 0
#define __BOUND_INDEX 1
#define __KEY_INDEX 10000000
#define __LOCK_INDEX 10000000

#elif __SOFTBOUNDCETS_TEMPORAL
    size_t key;
    void *lock;
#define __SOFTBOUNDCETS_METADATA_NUM_FIELDS 2
#define __KEY_INDEX 0
#define __LOCK_INDEX 1
#define __BASE_INDEX 10000000
#define __BOUND_INDEX 10000000

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    void *base;
    void *bound;
    size_t key;
    void *lock;
#define __SOFTBOUNDCETS_METADATA_NUM_FIELDS 4

#define __BASE_INDEX 0
#define __BOUND_INDEX 1
#define __KEY_INDEX 2
#define __LOCK_INDEX 3

#endif

} __softboundcets_trie_entry_t;

//===----------------------------------------------------------------------===//
//                              TODO
//===----------------------------------------------------------------------===//

#if ENABLE_RT_STATS
void __rt_stat_inc_sb_access_check(void);

void __rt_stat_inc_sb_mem_check(void);

void __rt_stat_inc_external_check(void);
#endif

#ifdef __SOFTBOUNDCETS_DEBUG
#undef __SOFTBOUNDCETS_DEBUG
static const int __SOFTBOUNDCETS_DEBUG = 1;
#define __SOFTBOUNDCETS_NORETURN
#else
static const int __SOFTBOUNDCETS_DEBUG = 0;
#define __SOFTBOUNDCETS_NORETURN __attribute__((__noreturn__))
#endif

static const size_t __SOFTBOUNDCETS_N_TEMPORAL_ENTRIES =
    ((size_t)64 * (size_t)1024 * (size_t)1024);
static const size_t __SOFTBOUNDCETS_LOWER_ZERO_POINTER_BITS = 3;

static const size_t __SOFTBOUNDCETS_N_STACK_TEMPORAL_ENTRIES =
    ((size_t)1024 * (size_t)64);
static const size_t __SOFTBOUNDCETS_N_GLOBAL_LOCK_SIZE =
    ((size_t)1024 * (size_t)32);

// 2^23 entries each will be 8 bytes each
static const size_t __SOFTBOUNDCETS_TRIE_PRIMARY_TABLE_ENTRIES =
    ((size_t)8 * (size_t)1024 * (size_t)1024);

static const size_t __SOFTBOUNDCETS_SHADOW_STACK_ENTRIES =
    ((size_t)128 * (size_t)32);

/* 256 Million simultaneous objects */
static const size_t __SOFTBOUNDCETS_N_FREE_MAP_ENTRIES =
    ((size_t)32 * (size_t)1024 * (size_t)1024);
// each secondary entry has 2^ 22 entries
static const size_t __SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES =
    ((size_t)4 * (size_t)1024 * (size_t)1024);

extern __softboundcets_trie_entry_t **__softboundcets_trie_primary_table;

extern size_t *__softboundcets_shadow_stack_ptr;
extern size_t *__softboundcets_temporal_space_begin;

extern size_t *__softboundcets_stack_temporal_space_begin;
extern size_t *__softboundcets_free_map_table;

extern size_t *__softboundcets_global_lock;

#if NOERRORS
extern void __softboundcets_abort();
#else
extern __SOFTBOUNDCETS_NORETURN void __softboundcets_abort();
#endif

extern void __softboundcets_printf(const char *str, ...);

__WEAK_INLINE void
__softboundcets_allocation_secondary_trie_allocate(void *addr_of_ptr);

#endif
