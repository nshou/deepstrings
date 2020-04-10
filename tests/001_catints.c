#include <stdio.h>

int b = 0x57202c6f;
static int d = 0x00000a21;

int main(void){
    int c = 0x646c726f;
    static int a = 0x6c6c6548;
    char s[16];

    *((int *)s) = a;
    *((int *)(s + 4)) = b;
    *((int *)(s + 8)) = c;
    *((int *)(s + 12)) = d;

    printf("%s", s);
    return 0;
}
