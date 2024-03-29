#include <stdio.h>
int main() {
    char* s1 = "Hello There";
    char* s2 = "ooooo ooooo";
    char i;
    printf("Enter string s1: ");

    while (s1[i] != '\0') {
        s2[i] = s1[i];
        i++;
    }

    s2[i] = '\0';
    printf("String s2: %s", s2);
    return 0;
}
