#include <stdlib.h>
int* foo(int *a) {
    return a;
}

int main() {
    int* t = malloc(8);
    t[0] = 5;
    t[1] = 7;

    int s = 0;

    s += (foo(t))[0];
    s += (foo(t) + 1)[0];

    free(t);

    return s;
}
