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

#ifndef SOFTBOUNDCETS_DEFINES_H
#define SOFTBOUNDCETS_DEFINES_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

// Ensure that we work on a 64bit system
static_assert(__WORDSIZE == 64, "This library is coded for 64bit systems only");

// Make sure that our comparisons are sound
static_assert(sizeof(uintptr_t) >= sizeof(size_t),
              "This library might compare objects of size_t with pointer "
              "values before applying arithmetic and therefore cast from "
              "size_t to uintptr_t, if unintptr_t is smaller than size_t, the "
              "program will not work as expected.");

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

// Option to generate debug output. Note that this can be very verbose if the
// instrumented programs accesses a lot of memory.
#ifndef __SOFTBOUNDCETS_DEBUG
#define __SOFTBOUNDCETS_DEBUG 0
#endif

// Option to allow running in the llvm test-suite framework. The test-suite
// unfortunately builds its utils with the compiler under test, which will not
// work with this instrumentation. Enabling this option allows you to skip the
// `timeit` module, which avoids crashing all tests at run-time.
#ifndef __SOFTBOUNDCETS_LLVM_TESTSUITE
#define __SOFTBOUNDCETS_LLVM_TESTSUITE 0
#endif

// In case zero bytes should be accessed, ignore the fact that the accessed
// pointer is not within valid allocation bounds or a nullptr.
#ifndef __SOFTBOUNDCETS_ALLOWWIDTHZEROACCESS
#define __SOFTBOUNDCETS_ALLOWWIDTHZEROACCESS 0
#endif

// The standard library wrappers have two purposes:
// First, they make sure to properly propagate metadata. If a standard library
// function returns a pointer, we need the metadata information of this pointer
// to properly check accesses to this pointer afterwards.
// Second, we can make sure that the calls to standard library functions will
// not result in undefined behavior. For example, we can check that the
// destination of a memcpy is large enough to store the bytes that should be
// copied to it.
//
// This flag allows you to enable/disable the checks on the arguments for
// standard library functions. Disabling them will result in finding less bugs.
// However, it might be useful for a fair run-time comparision against
// approaches that do not offer these checks.
#ifndef __SOFTBOUNDCETS_WRAPPER_CHECKS
#define __SOFTBOUNDCETS_WRAPPER_CHECKS 0
#endif

// Option to not report errors if a bound value is null [testing only]
#ifndef __SOFTBOUNDCETS_NOERRORMISSINGBOUNDS
#define __SOFTBOUNDCETS_NOERRORMISSINGBOUNDS 0
#endif

// Option for printing statistics about dynamically executed checks [testing
// only]
#ifndef __SOFTBOUNDCETS_ENABLE_RT_STATS
#define __SOFTBOUNDCETS_ENABLE_RT_STATS 0
#endif

// TODO add description of this option
#ifndef __SOFTBOUNDCETS_PREALLOCATE_TRIE
#define __SOFTBOUNDCETS_PREALLOCATE_TRIE 0
#endif

// TODO add description of this option
// #define __SOFTBOUNDCETS_CONSTANT_STACK_KEY_LOCK

// TODO add description of this option
// #define __NOSIM_CHECKS

// The free map stores information on pointers that can be freed (those
// dynamically allocated by malloc/calloc etc.). This should (always) be enabled
// when using temporal safety.
// TODO Unsure why this flag is necessary, I don't see a reason why someone
// should disable the free map (apart from testing maybe?)
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

#define __SOFTBOUNDCETS_NORETURN __attribute__((__noreturn__))

#if defined(__APPLE__)
#define SOFTBOUNDCETS_MMAP_FLAGS (MAP_ANON | MAP_NORESERVE | MAP_PRIVATE)
#else
#define SOFTBOUNDCETS_MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE)
#endif

//===----------------------------------------------------------------------===//
//                            Data structures
//===----------------------------------------------------------------------===//

// Define key and lock type for nicer usability of them
typedef size_t key_type;
typedef void *lock_type;

// Trie represented by the following by a structure with four fields if both
// __SOFTBOUNDCETS_SPATIAL and __SOFTBOUNDCETS_TEMPORAL are specified.
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
    key_type key;
    lock_type lock;
#define __SOFTBOUNDCETS_METADATA_NUM_FIELDS 2
#define __KEY_INDEX 0
#define __LOCK_INDEX 1
#define __BASE_INDEX 10000000
#define __BOUND_INDEX 10000000

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    void *base;
    void *bound;
    key_type key;
    lock_type lock;
#define __SOFTBOUNDCETS_METADATA_NUM_FIELDS 4

#define __BASE_INDEX 0
#define __BOUND_INDEX 1
#define __KEY_INDEX 2
#define __LOCK_INDEX 3

#endif

} __softboundcets_trie_entry_t;

//===----------------------------------------------------------------------===//
//                    Commonly used internal functions
//===----------------------------------------------------------------------===//

#if __SOFTBOUNDCETS_ENABLE_RT_STATS
void __rt_stat_inc_sb_access_check(void);

void __rt_stat_inc_sb_mem_check(void);

void __rt_stat_inc_external_check(void);
#endif

extern void __softboundcets_printf(const char *str, ...);

#if __SOFTBOUNDCETS_DEBUG
#define __softboundcets_debug_printf(...) __softboundcets_printf(__VA_ARGS__)
#else
#define __softboundcets_debug_printf(...)
#endif

extern __SOFTBOUNDCETS_NORETURN void __softboundcets_abort();
extern __SOFTBOUNDCETS_NORETURN void
__softboundcets_abort_with_msg(const char *str);

typedef size_t *shadow_stack_ptr_type;

// The current level of the shadow stack
extern shadow_stack_ptr_type __softboundcets_shadow_stack_ptr;

// The limit of the shadow stack
extern shadow_stack_ptr_type __softboundcets_shadow_stack_max;

#endif // SOFTBOUNDCETS_DEFINES_H
