#include <stdio.h>

int main(void){
    long local1 = 0x0a21646c726f;
    int local2 = 0x57202c6f;
    int local3 = 0x6c6c6548;
    long local4 = 0;

    (void)local1;
    (void)local2;
    (void)local3;
    printf("%s", ((char *)&local4) + 8);
    return 0;
}
