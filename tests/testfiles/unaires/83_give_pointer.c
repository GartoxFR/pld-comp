void incr(int* x) {
    (*x)++;
}

int main() {
    int x = 41;
    incr(&x);
    return x;
}
