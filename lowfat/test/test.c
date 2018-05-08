#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "check.h"

int main(void)
{
    printf("foo\n");
    int size = 8;
    int* p = (int*) malloc(size * sizeof(int));

    uintptr_t p_size = _ptr_size(p);
    uintptr_t p_base = _ptr_base(p);
    __check_deref(p + 8, p_size, p_base);

    // check if some library (like mmap) had errors
    printf("%s\n", strerror(errno));

    /* free(p); */
    printf("bar\n");
    return 0;
}
