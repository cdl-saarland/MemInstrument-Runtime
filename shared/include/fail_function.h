#pragma once

/**
 * This file contains declarations for different fail functions that abort
 * program execution and print a nice message.
 *
 * It is expected that the instrumentation mechanism provides a "config.h"
 * header with #defines that control the behavior of the instrumentation.
 * The following macros are relevant:
 *      SILENT - Defining this macro disables any messages.
 *      CONTINUE_ON_FATAL - Defining this macro disables termination of
 *          execution.
 *      FAIL_COUNTER - If defined, the value of this macro is used as a
 *          statistic counter that is incremented on each call of a fail
 *          function. This will only have a value differing from 0 or 1 if
 *          CONTINUE_ON_FATAL is defined.
 *      RET_FAIL - The exit code that is produced by the fail functions (73 is
 *          used if this macro is not defined).
 *      MAX_BACKTRACE_LENGTH - Determines the maximal number of functions from
 *          the call stack to print in the backtrace on abort.
 */

#ifndef CONTINUE_ON_FATAL
_Noreturn
#endif
void __mi_fail_fmt(FILE* dest, const char* fmt, ...);

#ifndef CONTINUE_ON_FATAL
_Noreturn
#endif
void __mi_fail(void);

#ifndef CONTINUE_ON_FATAL
_Noreturn
#endif
void __mi_fail_with_msg(const char* msg);

#ifndef CONTINUE_ON_FATAL
_Noreturn
#endif
void __mi_fail_with_ptr(const char* msg, void *faultingPtr);

#ifndef CONTINUE_ON_FATAL
_Noreturn
#endif
void __mi_fail_verbose_with_ptr(const char* msg, void *faultingPtr, const char* verbMsg);

