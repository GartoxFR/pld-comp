int min(int x, int y) {
    if (x > y) {
        return y;
    }

    return x;
}

int ppcd(int x, int y) {
    int mini = min(x, y);
    int ppcd = 2;

    while (ppcd < mini && x % ppcd != 0 && y % ppcd != 0) {
        ppcd = ppcd + 1;
    }

    if (x % ppcd != 0 && y % ppcd != 0) {
        ppcd = -1;
    }

    return ppcd;
}

int main() {
    int x = 11, y = 31;

    int res = ppcd(x, y);

    return res;
}