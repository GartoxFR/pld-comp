#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int somme(int x, int y){
    printf("%d", x+y);
    return x + y;
}

void rien(){
    
}

int main(){
    int a = 1;
    printf("%d", a);
    int b = 'a';
    printf("%d", b);
    int c = a + b - 2*a/b%3;
    printf("%d", c);
    int d = a | (b & (c ^ a));
    printf("%d", c);
    bool e = a == b;
    bool f = a <= b;
    bool g = a >= b;
    bool h = a != b;
    bool i = a < b;
    bool j = a > b;

    int* z = malloc(2 * 4);
    z[0] = 5;
    printf("%d\n", z[0]);
    z[1] = 10;
    printf("%d\n", z[1]);
    int r = z[0] + z[1];
    printf("%d", r);

    free(z);
    a = !b;
    printf("%d", a);
    d = -a;
    printf("%d", d);
    putchar('H');
    somme(a,d);
    rien();
    {
        int a = 6;
        printf("%d", a);
    }
    a = 5;
    printf("%d", a);
    a += 5;
    printf("%d", a);
    a -= 5;
    printf("%d", a);

    char* y = malloc(2*4);
    y[0] = 'a';
    printf("%c", y[0]);
    y[1] = 'b';
    printf("%c", y[1]);


    if(a == 5 && b == 5){
        printf("%d", a*b);
        return a*b;
    } else if (a == 4 || b == 4){
    }

    return a;
}

