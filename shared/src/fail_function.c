#include "fail_function.h"

#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "statistics.h"

#ifndef UNUSED
#define UNUSED(x) (void)x;
#endif

#ifndef MIRT_MAX_BACKTRACE_LENGTH
#define MIRT_MAX_BACKTRACE_LENGTH 16
#endif

#define PRINTBACKTRACE                                                         \
    {                                                                          \
        void *buf[MIRT_MAX_BACKTRACE_LENGTH];                                  \
        int n = backtrace(buf, MIRT_MAX_BACKTRACE_LENGTH);                     \
        backtrace_symbols_fd(buf, n, STDERR_FILENO);                           \
    }

MIRT_NO_RETURN void __mi_fail_fmt(FILE *dest, const char *fmt, ...) {
#ifdef MIRT_FAIL_COUNTER
    STAT_INC(MIRT_FAIL_COUNTER);
#endif
#ifdef MIRT_SILENT
    UNUSED(dest)
    UNUSED(fmt)
#endif

    va_list args;
    va_start(args, fmt);

#ifdef MIRT_CONTINUE_ON_FATAL
#ifndef MIRT_SILENT
    fprintf(dest, "FATAL: Memory safety violation!");
    if (fmt) {
        fprintf(dest, " ");
        vfprintf(dest, fmt, args);
    }
    fprintf(dest, "\n");
#endif
#else
#ifndef MIRT_SILENT
    fprintf(dest, "Memory safety violation!\n"
                  "         / .'\n"
                  "   .---. \\/\n"
                  "  (._.' \\()\n"
                  "   ^\"\"\"^\"\n");
    vfprintf(dest, fmt, args);

    fprintf(stderr, "\n#################### meminstrument --- backtrace start "
                    "####################\n");
    fprintf(stderr, "> executable: %s\n", __get_prog_name());
    PRINTBACKTRACE;
    fprintf(stderr, "#################### meminstrument --- backtrace end "
                    "######################\n");
#endif

    abort();
#endif
}

MIRT_NO_RETURN void __mi_fail(void) { __mi_fail_fmt(stderr, NULL); }

MIRT_NO_RETURN void __mi_fail_with_msg(const char *msg) {
    __mi_fail_fmt(stderr, "%s", msg);
}

MIRT_NO_RETURN void __mi_fail_with_ptr(const char *msg, void *faultingPtr) {
    __mi_fail_fmt(stderr, "%s with pointer %p", msg, faultingPtr);
}

MIRT_NO_RETURN void __mi_fail_verbose_with_ptr(const char *msg,
                                               void *faultingPtr,
                                               const char *verbMsg) {
    __mi_fail_fmt(stderr, "%s with pointer %p\nat %s", msg, faultingPtr,
                  verbMsg);
}

void __mi_warning(const char *msg) {
    fprintf(stderr, "\nMI-WARNING: %s\n", msg);
}

void __mi_printf(const char *str, ...) {
    va_list args;

    va_start(args, str);
    vfprintf(stderr, str, args);
    va_end(args);
}
