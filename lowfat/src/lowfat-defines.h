#pragma once

#include <assert.h>
#include <stddef.h>

//===----------------------------------------------------------------------===//
//                       Configuration Options
//===----------------------------------------------------------------------===//

// All configuration options are described in the README.md, please refer to it
// for details on the configurations.

// We can determine the size by either using a table or computing the size.
// Ensure that exactly one is defined, default to the TABLE option.
#ifndef MIRT_LF_TABLE
#ifndef MIRT_LF_COMPUTED_SIZE
#define MIRT_LF_TABLE 1
#else
#define MIRT_LF_TABLE 0
#endif
#endif

#ifndef MIRT_LF_COMPUTED_SIZE
#define MIRT_LF_COMPUTED_SIZE 0
#endif

static_assert(!(MIRT_LF_TABLE && MIRT_LF_COMPUTED_SIZE),
              "Invalid configuration, either lookup tables or computations can "
              "be used to determine base/size.");

//===----------------------------------------------------------------------===//
//                              Shorthands
//===----------------------------------------------------------------------===//

#define __CHECK_ATTRIBUTES __attribute__((__always_inline__))
#define __INTERNAL_FUNCTIONS_ATTRIBUTES __attribute__((__always_inline__))

//===----------------------------------------------------------------------===//
//                              Assumptions
//===----------------------------------------------------------------------===//

static_assert(__WORDSIZE == 64, "This library is coded for 64bit systems only");

static_assert(sizeof(size_t) == 8,
              "The computation to derive the region index from the size of an "
              "allocation requires `size_t` to have 64 bit.");
