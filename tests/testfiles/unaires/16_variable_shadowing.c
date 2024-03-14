int main() {
    int x = 1;
    int y;
    {
        int x = 5;
        x = x;
        y = x;
    }

    return x + y;
}
