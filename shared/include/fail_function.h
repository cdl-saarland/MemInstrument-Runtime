#pragma once

#include <stdio.h>

/**
 * This file contains declarations for different fail functions that abort
 * program execution and print a nice message.
 *
 * It is expected that the instrumentation mechanism provides a "config.h"
 * header with #defines that control the behavior of the instrumentation.
 * The following macros are relevant:
 *      MIRT_SILENT - Defining this macro disables any messages.
 *      MIRT_CONTINUE_ON_FATAL - Defining this macro disables termination of
 *          execution.
 *      MIRT_FAIL_COUNTER - If defined, the value of this macro is used as a
 *          statistic counter that is incremented on each call of a fail
 *          function. This will only have a value differing from 0 or 1 if
 *          MIRT_CONTINUE_ON_FATAL is defined.
 *      MIRT_RET_FAIL - The exit code that is produced by the fail functions (73
 *          is used if this macro is not defined).
 *      MIRT_MAX_BACKTRACE_LENGTH - Determines the maximal number of functions
 * from the call stack to print in the backtrace on abort.
 */

#ifndef MIRT_NO_RETURN
#ifdef MIRT_CONTINUE_ON_FATAL
#define MIRT_NO_RETURN
#else
#define MIRT_NO_RETURN _Noreturn
#endif
#endif

MIRT_NO_RETURN void __mi_fail_fmt(FILE *dest, const char *fmt, ...);

MIRT_NO_RETURN void __mi_fail(void);

MIRT_NO_RETURN void __mi_fail_with_msg(const char *msg);

MIRT_NO_RETURN void __mi_fail_with_ptr(const char *msg, void *faultingPtr);

MIRT_NO_RETURN void __mi_fail_verbose_with_ptr(const char *msg,
                                               void *faultingPtr,
                                               const char *verbMsg);

void __mi_warning(const char *msg);
