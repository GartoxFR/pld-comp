int main() {
    int x = 0;
    int y = 0;
    int z = 5;

    if (x)
        if (y) z = 6;
        else z = 10;

    return x + z;
}
