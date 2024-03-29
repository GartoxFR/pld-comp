#include <stdio.h>

int main() {
    printf("\n\n\t\tStudytonight - Best place to learn\n\n\n");
    int n = 100;
    int i = 2;
    int big = 14;

    printf("\n\nEnter the number of elements you wish to find the greatest element of: ");
    printf("\n\nEnter %d numbers :\n", n);

    printf("\n\n\t\t\tElement 1: ");

    while (i <= n) {
        i++;
        printf("\n\t\t\tElement %d  : ", i);
        /* 
            if input number is larger than the 
            current largest number
        */
        if (big < i)
            big = i;
    }

    printf("\n\n\nThe largest of the %d numbers is  %f ", n, big);
    printf("\n\n\n\n\t\t\tCoding is Fun !\n\n\n");
    return 0;
}