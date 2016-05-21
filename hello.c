#include <stdio.h>

int fibonacci( int i ){
    if(i <= 1) {
        return i;
    }
    return fibonacci(i - 1) + fibonacci(i - 2);
}

int main(){
    int i;
    int b;
    b = 0;
    i = 0;
    while(i <= 10) {
        printf("fibonacci(%2d) = %d\n", i, fibonacci(i));
        i = i + 1;
    }
    printf("3 * (2 + 2) = %d\n", 3*(2+2));
    b = b == 0 ? 1 : 10;
    printf("after b == 0 ? 1 : 10, b = %d\n", b);
    return 0;
}
