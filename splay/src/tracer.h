#pragma once

#include <stdint.h>

void tracerInit(const char* binname, const char* outfile);

void tracerStartOp(void);

void tracerRegisterDelete(uint64_t lb, uint64_t ub);

void tracerRegisterInsert(uint64_t lb, uint64_t ub, const char* data);

void tracerSetData(const char* data);

void tracerEndOp(void);


void tracerRegisterCheck(uint64_t lb, uint64_t ub, const char* data);

