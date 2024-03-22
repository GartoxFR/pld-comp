char test(short x, char y) {
    return x / y;
}

long test2(long x, int y) {
    return x + y * 2;
}

int test2(char x, short y) {
    return (x & 3 + y) ^ 2;
}

int main(){
    return test(3, 2);
}
