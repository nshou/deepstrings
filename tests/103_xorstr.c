#include <stdio.h>

char *key = "SECRET";

int main(void){
    char e[] = {0x1b, 0x20, 0x2f, 0x3e, 0x2a, 0x78, 0x73, 0x12, 0x2c, 0x20, 0x29, 0x30, 0x72, 0x4f, 0x43};
    int i;

    for(i = 0; (e[i] = e[i] ^ key[i % 6]) != 0; i++);

    printf("%s", e);
    return 0;
}