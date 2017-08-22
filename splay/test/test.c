#include <stdlib.h>
#include <stdio.h>


int main(void)
{
    printf("foo\n");
    int size = 7;
    int* p = (int*) malloc(size * sizeof(int));

    for (int i = 0; i < size; ++i) {
        p[i] = 42+i;
    }

    for (int i = 0; i < size; ++i) {
        int* q = malloc(p[i]);
        q[0] = 5;
    }

    /* free(p); */
    printf("bar\n");
    return 0;
}
