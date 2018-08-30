#include "tracer.h"
#include <assert.h>
#include <stdio.h>

#include <execinfo.h>
#include <unistd.h>

static uint64_t running_index = 0;

static FILE *OutFile = NULL;

void tracerInit(const char *binname, const char *outfile) {
    OutFile = fopen(outfile, "a");
    assert(OutFile);
    fprintf(OutFile, "TRACE %s\n", binname);
}

void tracerStartOp(void) {
    fprintf(OutFile, "BEGINENTRY(%lu)\n", running_index);
}

void tracerRegisterDelete(uint64_t lb, uint64_t ub) {
    fprintf(OutFile, "  DELETE(%lu) %#lx %#lx\n", running_index, lb, ub);
}

void tracerRegisterInsert(uint64_t lb, uint64_t ub, const char *data) {
    fprintf(OutFile, "  INSERT(%lu) %#lx %#lx '%s'\n", running_index, lb, ub,
            data);
}

void tracerSetData(const char *data) {
    fprintf(OutFile, "  DATA(%lu) '%s'\n", running_index, data);
}

void tracerEndOp(void) {
    int btlen = 10;
    fprintf(OutFile, "  BACKTRACE(%lu)\n", running_index);
    fflush(OutFile);
    void *buf[btlen];
    int n = backtrace(buf, btlen);
    backtrace_symbols_fd(buf, n, fileno(OutFile));
    fprintf(OutFile, "  ENDBACKTRACE(%lu)\n", running_index);

    fprintf(OutFile, "ENDENTRY(%lu)\n\n", running_index);
    ++running_index;
}

void tracerRegisterCheck(uint64_t lb, uint64_t ub, const char *data) {
    fprintf(OutFile, "BEGINCHECK(%lu)\n", running_index);
    fprintf(OutFile, "  LOWER(%lu) %#lx\n", running_index, lb);
    fprintf(OutFile, "  UPPER(%lu) %#lx\n", running_index, ub);
    if (data) {
        fprintf(OutFile, "  DATA(%lu)  %s\n", running_index, data);
    }
    int btlen = 10;
    fprintf(OutFile, "  BACKTRACE(%lu)\n", running_index);
    fflush(OutFile);
    void *buf[btlen];
    int n = backtrace(buf, btlen);
    backtrace_symbols_fd(buf, n, fileno(OutFile));
    fprintf(OutFile, "  ENDBACKTRACE(%lu)\n", running_index);

    fprintf(OutFile, "ENDCHECK(%lu)\n\n", running_index);
    ++running_index;
}
