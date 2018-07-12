#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

void __splay_check_dereference(void* witness, void* ptr, size_t sz);

void foo(int*p, int size) {
    for (int i = 0; i < size; ++i) {
        __splay_check_dereference(p, p+i, sizeof(int));
        p[i] = 42+i;
    }
}

int main(void)
{
    printf("foo\n");
    int size = 7;
    int* p = (int*) malloc(size * sizeof(int));

    foo(p, size);

    for (int i = 0; i < size; ++i) {
        int* q = aligned_alloc(16, p[1]);
        q[0] = 5;
    }

    /* free(p); */
    printf("bar\n");
    return 0;
}
