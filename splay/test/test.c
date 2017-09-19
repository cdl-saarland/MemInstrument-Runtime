#include <stdlib.h>
#include <stdio.h>



void __splay_check_dereference(void* witness, void* ptr, size_t sz);

void foo(int*p, int size) {
    for (int i = 0; i < size+1; ++i) {
        p[i] = 42+i;
        __splay_check_dereference(p, p+i, sizeof(int));
    }
}

int main(void)
{
    printf("foo\n");
    int size = 7;
    int* p = (int*) malloc(size * sizeof(int));

    foo(p, size);

    for (int i = 0; i < size; ++i) {
        int* q = malloc(p[i]);
        q[0] = 5;
    }

    /* free(p); */
    printf("bar\n");
    return 0;
}
