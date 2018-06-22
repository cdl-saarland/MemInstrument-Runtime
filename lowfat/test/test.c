#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "check.h"

void test_realloc(void) {
    int* p = malloc(16);
    *p = 5;
    p = realloc(p, 16);
    p = realloc(p, 32);
    p = realloc(p, 2000);
    p = realloc(p, 16);

    if (*p != 5 || p != 0x100000000)
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

    uintptr_t p_size = __lowfat_ptr_size(p);
    uintptr_t p_base = __lowfat_ptr_base(p);
    __lowfat_check_oob(p + 8, p_size, p_base);

    printf("OOB detection failed");
}

struct TABLE {
    unsigned size;
    uint64_t inv_size;
};

void mmap_metadata(void) {
    int fd = open("/home/philip/testfile", O_RDONLY);
    struct TABLE *test = mmap(NULL, 1, PROT_READ, MAP_PRIVATE, fd, 0);

    FILE *f = fopen("/home/philip/meta_standard_mmap", "w");
    unsigned size = 16;
    unsigned i = 1;
    while (size < 2000000000) {
        struct TABLE a;
        a.size = size;
        a.inv_size = UINT64_MAX / size + 1;
        fwrite(&a, 16, 1, f);
        if (size < 8192)
            size += 16;
        else
            size *= 2;
        i++;
    }
}

int main(void)
{
    int* p=malloc(1);
    // check if some library (like mmap/mprotect) had errors
    printf("ERRNO: %s\n", strerror(errno));
    return 0;
}