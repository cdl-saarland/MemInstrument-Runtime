#pragma once


// SILENT
// FAIL_COUNTER
// CONTINUE_ON_FATAL
// RET_FAIL

_Noreturn void __mi_fail(void);

_Noreturn void __mi_fail_with_msg(const char* msg);

_Noreturn void __mi_fail_with_ptr(const char* msg, void *faultingPtr);

_Noreturn void __mi_fail_verbose_with_ptr(const char* msg, void *faultingPtr, const char* verbMsg);
