int calc(int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9, char x10) {
    int a = x1 + x2;
    int b = a * x3;
    int c = x4 * x5;
    int d = x6 / x7;
    int e = x8 % x10;
    int f = x10 | x1;

    return a + b + c + d + e + f;
}

int main() {
    return calc(256, 5674, 3241564, 6489, 65489, 964, 354, 321, 364, 123);
}
