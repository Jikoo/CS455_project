#ifndef SECURITY_H
#define SECURITY_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/types.h>

void generate_salt(unsigned char buf[8]);

void generate_iv(unsigned char buf[32]);

unsigned char* calculate_hash(char *input, unsigned char salt[8]);

unsigned char* log_in(char *password, unsigned char salt[8], unsigned char hash[SHA256_DIGEST_LENGTH]);

// TODO should probably change key and IV to fixed len
//void cipher(unsigned char *in, int len, FILE *out, unsigned char *key, unsigned char *iv, int enc);
int cipher(unsigned char *in, int len, FILE *out, unsigned char *key, unsigned char *iv, int enc);

int generate_key_iv(unsigned char *key, unsigned char *iv);
#endif
