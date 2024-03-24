#include <stdlib.h>

int main(){
    int* x = malloc(4);

    *x = 5;
    *x += 5;

    int y = *x;
    free(x);
    return y;
}
