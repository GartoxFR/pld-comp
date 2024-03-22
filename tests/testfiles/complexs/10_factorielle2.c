#include <stdio.h>

int fact(int x) {
    int res = 1;
    while (x-- > 1) {
        putchar(x + '0');
        res *= x;
    }

    return res;
}

int main() {
    return fact(10);
}
