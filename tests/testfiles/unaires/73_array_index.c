#include <stdlib.h>
#include <stdio.h>

int main(){
    int* x = malloc(2 * 4);

    x[0] = 5;
    printf("%d", x[0]);
    x[1] = 10;
    printf("%d", x[1]);
    int r = x[0] + x[1];
    printf("%d", r);
    free(x);
    return r;
}
