#include <stdlib.h>
#include <stdio.h>


int main(void)
{
    printf("foo\n");
    int size = 7;
    int* p = (int*) calloc(size, sizeof(int));

    for (int i = 0; i < size; ++i) {
        p[i] = 42+i;
    }

    for (int i = 0; i < size; ++i) {
        int* q = malloc(p[i]);
        free(q);
    }

    int* q =realloc(p, size + sizeof(int));

    free(q);
    printf("bar\n");
    return 0;
}
