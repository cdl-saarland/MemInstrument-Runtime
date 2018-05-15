#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "check.h"

int main(void)
{
    int size = 8;
    int* p = (int*) malloc(size * sizeof(int));
    *p = 5;
    int* q = (int*) malloc(size * sizeof(int));
    free(p);
    free(q);
    p = (int*) malloc(size * sizeof(int));
    free(p);

    // check if some library (like mmap/mprotect) had errors
    printf("%s\n", strerror(errno));
    return 0;
}

// access on OOB element
void test_basic_check(void) {
    int size = 8;
    int* p = (int*) malloc(size * sizeof(int));
    free(p);
    p = (int*) malloc(size * sizeof(int));

    uintptr_t p_size = _ptr_size(p);
    uintptr_t p_base = _ptr_base(p);
    __check_deref(p + 8, p_size, p_base);

    printf("OOB detection failed");
}