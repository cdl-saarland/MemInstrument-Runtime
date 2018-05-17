#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "check.h"

int main(void)
{
    test_basic_realloc();

    // check if some library (like mmap/mprotect) had errors
    printf("ERRNO: %s\n", strerror(errno));
    return 0;
}

void test_basic_realloc(void) {
    int* p = (int*) malloc(8);
    p = (int*) realloc(p, 32);
    if (p != 0x200000000)
        printf("unexpected realloc behavior");
}

void test_overflow(void) {
    for (int i = 0; i < 0xFFFFFFF; i++)
        malloc(16);

    int* p = (int*) malloc(16);
    int* q = (int*) malloc(16);

    if (q >= 0x100000000 && q <= 0x200000000)
        printf("unexpected overflow behavior");
}

void test_basic_free(void) {
    int size = 8;
    int* p = (int*) malloc(size * sizeof(int));
    *p = 5;
    int* q = (int*) malloc(size * sizeof(int));
    free(p);
    free(q);
    p = (int*) malloc(size * sizeof(int));
    free(p);

    if (p != 0x200000020)
        printf("unexpected pointer address for malloc after free");
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