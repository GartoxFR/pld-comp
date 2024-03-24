#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main() {
    int size = 300;

    bool* prime = malloc(size);

    prime[0] = 0;
    prime[1] = 0;

    int i = 2;
    while (i < size) {
        prime[i] = 1;
        i++;
    }

    int count = 0;

    i = 2;
    while (i < size) {
        if (!prime[i]) {
            i++;
            continue;
        }

        printf("%d\n", i);
        count++;

        int j = i;
        while (j < size) {
            prime[j] = 0;
            j += i;
        }

        i++;
    }

    return count;
}
