#include <stdio.h>

void reverseSentence(char* sentence, int i) {
    char c;
    c = sentence[i];
    i++;
    if (c != '\0') {
        reverseSentence(sentence, i);
        printf("%c", c);
    }
}

int main() {
    char* sentence = "Hello World";
    reverseSentence(sentence, 0);
    return 0;
}
