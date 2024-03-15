int main(){
    int x = 0;
    int y = 0; 

    if (x == 0 || (x = x + 1) == 10) {
        y = y + 5;
    }

    if (x == 1 || (x = x + 1) == 1) {
        y = y + 6;
    }

    return x + y;
}
