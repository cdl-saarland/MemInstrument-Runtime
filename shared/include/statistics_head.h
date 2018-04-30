/**
 * This file contains the generator for a statistics header file.
 *
 * It is used as follows for a "foo" instrumentation:
 *
 * Create a foo_statistics.h file somewhere in the foo subfolder.
 * In there, define STAT_COUNTER_FILE to the name of a file that contains
 * entries of the form
 *      STAT_ACTION(CounterName, "Counter description")
 * Furthermore, define the STATISTICS variable if statistics should be produced.
 * You might also consider setting the STATS_FILE variable to a file name to
 * force statistic printing to this file instead of stderr.
 *
 * To initialize the statistics mechanism, include foo_statistics.h and call
 * __setupt_statistics() with the name of the binary (this expanded to its
 * absolute path automatically). This also registers the statistics printer to
 * run at the very end of the program.
 *
 * To increment a counter with name "Bar" (as defined in the STAT_COUNTER_FILE),
 * include foo_statistics.h and use
 *      STAT_INC(Bar);
 * If STATISTICS is not defined, this is a no-op.
 *
 * Independently of whether STATISTICS is set, foo_statistics.h declares the
 * __get_prog_name() function that returns the absolute path of the binary as
 * a string.
 *
 * Make sure to also create a statistics implementation file!
 */

void __setup_statistics(const char* n);

const char* __get_prog_name(void);

#ifdef STATISTICS

#include <stddef.h>

// Declare all statistics counters
#define STAT_ACTION(var, text) \
    extern size_t __##var;

#include STAT_COUNTER_FILE

#undef STAT_ACTION

#define STAT_INC(var) { ++ __##var; }

#else

#define STAT_INC(var) {;}

#endif
