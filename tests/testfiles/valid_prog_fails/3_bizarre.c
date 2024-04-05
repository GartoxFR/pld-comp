int doThings(int a) {
    int c = 3;
    while (c % 2 != 0) {
        c = a + c;
    }

    int j = 39478;

    return j % c;
}

int main() {
    int a = 23;
    int b;

    while (a > 0) {
        b = doThings(a);

        a -= b;
    }

    return a * a;
}