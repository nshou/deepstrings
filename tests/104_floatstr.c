#include <stdio.h>

long val1 = 0xDEAD0a21646c726f;
long val2 = 0x57202c6f6c6c6548;

int main(void){
    char c[16];
    char *d = c;
    *((long *)c) = val2;
    *((long *)(c + 8)) = val1;

    while(*((unsigned short *)d) != 0xDEAD){
        printf("%c", *d++);
    }

    return 0;
}
