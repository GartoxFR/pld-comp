void incr(int* x, int* y) {
    *x += *y;
    *x += *y;
}

int main() {
    int x = 15;
    incr(&x, &x);
    return x;
}
