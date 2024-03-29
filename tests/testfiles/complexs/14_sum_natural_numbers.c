#include <stdio.h>
int main() {
    int n = 100;
    int i = 1;
    int sum = 0;

    printf("Enter a positive integer: ");

    while (i <= n) {
        i++;
        sum += i;
    }

    printf("Sum = %d", sum);
    return 0;
}
