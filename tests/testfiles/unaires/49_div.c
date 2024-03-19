int div(int x, int y) {
    return x / y;
}

int div2(int x) {
    return x / 2;
}

int main(){
    return div(4, 2) + div(3, 2) + div(4, 10) + div(21, 4) + div2(33);
}
