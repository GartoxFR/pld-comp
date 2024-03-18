int main(){
    int x = 10;
    int s = 0; 

    while (x > 0) {
        if (x == 5) {
            x = x - 1;
            continue;
        }

        s = s + x;
        x = x - 1;
    }

    return s;
}
