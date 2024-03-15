int main(){
    int x = 0;
    int y = 0; 

    if (x == 1 && (x = x + 1) == 2) {
        y = 5;
    }

    if (x == 0 && (x = x + 2) == 3) {
        y = 6;
    }

    return x + y;
}
