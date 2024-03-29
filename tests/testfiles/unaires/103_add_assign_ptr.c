#include <stdlib.h>

int main() {
    int* table = malloc(4 * 4);
    int* cursor = table;
    int** c = table;

    table[0] = 5;
    table[1] = 9;
    table[2] = 1;
    table[3] = -2;

    int s = **c;
    s += *(*c += 1);
    s += *(*c += 1);
    s += *(*c += 1);

    free(table);
    
    return s;
}
