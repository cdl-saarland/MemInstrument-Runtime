#include "tracer.h"

#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "macros.h"

#ifdef ENABLE_TRACER
static uint64_t running_index = 0;

static FILE *OutFile = NULL;

#define dumpBackTrace(of)                                                      \
    {                                                                          \
        int btlen = MIRT_MAX_BACKTRACE_LENGTH;                                 \
        fprintf(of, "  BACKTRACE(%lu)\n", running_index);                      \
        fflush(of);                                                            \
        void *buf[btlen];                                                      \
        int n = backtrace(buf, btlen);                                         \
        backtrace_symbols_fd(buf, n, fileno(of));                              \
        fprintf(of, "  ENDBACKTRACE(%lu)\n", running_index);                   \
    }
#endif

void tracerInit(const char *binname) {
#ifdef ENABLE_TRACER
    OutFile = fopen(TRACER_FILE, "a");
    assert(OutFile);
    fprintf(OutFile, "TRACE %s\n", binname);
#else
    UNUSED(binname)
#endif
}

void tracerStartOp(void) {
#ifdef ENABLE_TRACER
    fprintf(OutFile, "BEGINENTRY(%lu)\n", running_index);
#endif
}

void tracerRegisterDelete(uint64_t lb, uint64_t ub) {
#ifdef ENABLE_TRACER
    fprintf(OutFile, "  DELETE(%lu) %#lx %#lx\n", running_index, lb, ub);
#else
    UNUSED(lb)
    UNUSED(ub)
#endif
}

void tracerRegisterInsert(uint64_t lb, uint64_t ub, const char *data) {
#ifdef ENABLE_TRACER
    fprintf(OutFile, "  INSERT(%lu) %#lx %#lx '%s'\n", running_index, lb, ub,
            data);
#else
    UNUSED(lb)
    UNUSED(ub)
    UNUSED(data)
#endif
}

void tracerSetData(const char *data) {
#ifdef ENABLE_TRACER
    fprintf(OutFile, "  DATA(%lu) '%s'\n", running_index, data);
#else
    UNUSED(data)
#endif
}

void tracerEndOp(void) {
#ifdef ENABLE_TRACER
    dumpBackTrace(OutFile);

    fprintf(OutFile, "ENDENTRY(%lu)\n\n", running_index);
    ++running_index;
#endif
}

void tracerRegisterCheck(uint64_t lb, uint64_t ub, const char *data) {
#ifdef ENABLE_TRACER
    fprintf(OutFile, "BEGINCHECK(%lu)\n", running_index);
    fprintf(OutFile, "  LOWER(%lu) %#lx\n", running_index, lb);
    fprintf(OutFile, "  UPPER(%lu) %#lx\n", running_index, ub);
    if (data) {
        fprintf(OutFile, "  DATA(%lu)  %s\n", running_index, data);
    }
    dumpBackTrace(OutFile);

    fprintf(OutFile, "ENDCHECK(%lu)\n\n", running_index);
    ++running_index;
#else
    UNUSED(lb)
    UNUSED(ub)
    UNUSED(data)
#endif
}
