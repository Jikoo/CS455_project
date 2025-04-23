#ifndef SECURITY_H
#define SECURITY_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/types.h>

void generate_salt(unsigned char ptr[8]);

unsigned char* calculate_hash(unsigned char *input, int len);

int log_in(char *password, char *salt, unsigned char hash[SHA256_DIGEST_LENGTH]);

void cipher(unsigned char *in, int len, FILE *out, unsigned char *key, unsigned char *iv, int enc);

#endif