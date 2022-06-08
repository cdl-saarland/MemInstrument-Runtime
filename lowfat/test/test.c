#include <stdlib.h>
#include <stdio.h>
#include "core.h"

// Test: Access on OOB element
int test_basic_check(void) {
    int size = 7;
    int* p = (int*) malloc(size * sizeof(int));
    free(p);
    p = (int*) malloc(size * sizeof(int));

    // Will go unnoticed due to rounding up the size to the next power of two
    __lowfat_check_oob(p, p + size);

    // Throws an error
    __lowfat_check_oob(p, p + size + 1);

    printf("OOB detection failed\n");
    return 1;
}

int main(void) {
    printf("\nIf this test succeeds, you will see a beautiful bug below!\n\n");
    return test_basic_check();
}
