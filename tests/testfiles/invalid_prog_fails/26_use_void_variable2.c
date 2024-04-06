#include <stdio.h>
void print() {
    printf("Hello world");
}

int main() {
    int x = print() + 5;
    return 0;
}
