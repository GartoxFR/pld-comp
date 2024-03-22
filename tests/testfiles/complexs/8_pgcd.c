int min(int x, int y) {
    if (x > y) {
        return y;
    }

    return x;
}

int pgcd(int x, int y) {
    int mini = min(x, y);
    int pgcd = 1;

    while (mini > 2 && pgcd == 1) {
        if (x % mini == 0 && y % mini == 0) {
            pgcd = mini;
        }
    }

    return pgcd;
}

int main() {
    int x = 10, y = 30;

    int res = pgcd(x, y);

    return res;
}