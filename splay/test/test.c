#include <stdlib.h>
#include <stdio.h>



void __splay_check_access(void* witness, void* ptr, size_t sz);

int main(void)
{
    printf("foo\n");
    int size = 7;
    int* p = (int*) malloc(size * sizeof(int));

    for (int i = 0; i < size; ++i) {
        p[i] = 42+i;
        __splay_check_access(p, p+i, sizeof(int));
    }

    for (int i = 0; i < size; ++i) {
        int* q = malloc(p[i]);
        q[0] = 5;
    }

    /* free(p); */
    printf("bar\n");
    return 0;
}
