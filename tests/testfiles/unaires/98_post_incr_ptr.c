#include <stdlib.h>

int main() {
    int* table = malloc(4 * 4);
    int* cursor = table;

    table[0] = 5;
    table[1] = 9;
    table[2] = 1;
    table[3] = -2;

    int s = *cursor++;
    s += *cursor++;
    s += *cursor++;
    s += *cursor++;

    free(table);
    
    return s;
}
