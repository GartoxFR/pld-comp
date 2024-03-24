#include <stdlib.h>

int main(){
    int* x = malloc(2 * 4);

    x[0] = 5;
    x[1] = 10;

    int r = x[0] + x[1];

    free(x);
    return r;
}
