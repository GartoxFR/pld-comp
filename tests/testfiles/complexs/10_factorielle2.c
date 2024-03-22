int fact(int x) {
    int res = 1;
    while (x-- > 1) {
        res *= x;
    }

    return res;
}

int main() {
    return fact(10);
}
