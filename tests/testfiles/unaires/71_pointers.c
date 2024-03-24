
int main(){
    int x = 0;
    int* y = &x;
    int* z = &*y;

    *y = 5;
    *z += 5;
    return x;
}
