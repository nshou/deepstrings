#include <openssl/evp.h>
#include <stdio.h>

int main(void){
    unsigned long ciphertext[2] = {0x7dd8877695b78cc7, 0x9f58303c25b4a952};
    unsigned char cleartext[32];
    unsigned char *key = (unsigned char *)"THE PERFECTLY SECURE SECRET KEY.";
    unsigned char *iv = (unsigned char *)"IV DOESNT MATTER";
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, cleartext, &len, (unsigned char *)ciphertext, sizeof(ciphertext));
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, cleartext + len, &len);
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    cleartext[plaintext_len] = 0;
    printf("%s\n", (char *)cleartext);

    return 0;
}
