int main() {
    int i = 10;
    int s = 0;

    while (i) {
        s = s + i;
        i = i - 1;
    }

    return s;
}
