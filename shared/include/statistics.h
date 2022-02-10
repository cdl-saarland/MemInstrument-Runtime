#pragma once

/**
 * This file contains declarations for statistic counters.
 *
 * The statistic counters are specified via a file in the directory of the
 * instrumentation mechanism.
 * It has to contain entries of the form
 *      STAT_ACTION(FooCounter, "Counter description")
 * for each counter "FooCounter".
 *
 * To initialize the statistics mechanism, include this file and call
 * __setup_statistics() with the name of the binary (this is expanded to its
 * absolute path automatically). This also registers the statistics printer to
 * run at the very end of the program.
 *
 * To increment a counter with name "Bar" (as defined in the stats counter
 * file), include this header and use
 *      STAT_INC(Bar);
 *
 * It is expected that the instrumentation mechanism provides a "config.h"
 * header with #defines that control the behavior of the instrumentation.
 * The following macros are relevant:
 *      MIRT_STATISTICS - Statistics are collected and printed iff this is
 *          defined.
 *      MIRT_STATS_FILE - If this is defined (as a string s), statistics will be
 *          printed (i.e. appended) to a file with the name s.
 *      MIRT_STATS_COUNTER_DEFS - Defines the name of the file in which counter
 *          definitions are given, should be in the include path.
 *
 * Independently of whether MIRT_STATISTICS is set, this header declares the
 * __get_prog_name() function that returns the absolute path of the binary as a
 * string.
 *
 * LLVM builds certain tooling functions with the compiler under test. It is not
 * advisable to instrument them, so if you use the LLVM setup, define
 * MIRT_IGNORE_LLVM_TOOLING.
 */

#include "config.h"

void __setup_statistics(const char *n);

// Determine whether the given function name belongs to one of the llvm testing
// tools.
int __is_llvm_tooling_function(const char *name);

const char *__get_prog_name(void);

#ifdef MIRT_STATISTICS

#include <stddef.h>

// Declare all statistics counters
#define STAT_ACTION(var, text) extern size_t __##var;

#include MIRT_STATS_COUNTER_DEFS

#undef STAT_ACTION

#define STAT_INC(var)                                                          \
    { ++__##var; }

#else

#define STAT_INC(var)                                                          \
    { ; }

#endif
