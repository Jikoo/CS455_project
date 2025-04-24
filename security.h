#ifndef SECURITY_H
#define SECURITY_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/types.h>

#define SALT_SIZE 8  // 64 bit salt
#define KEY_SIZE 32  // 256 bits for AES-256
#define IV_SIZE 16   // 128 bits for AES block size

void generate_salt(unsigned char buf[SALT_SIZE]);

void generate_iv(unsigned char buf[IV_SIZE]);

unsigned char* calculate_hash(const char *input, const unsigned char salt[SALT_SIZE]);

unsigned char* log_in(const char *password, const unsigned char salt[SALT_SIZE], const unsigned char hash[SHA256_DIGEST_LENGTH]);

int cipher(const unsigned char *in, const int len, FILE *out, const unsigned char key[KEY_SIZE], const unsigned char iv[IV_SIZE], int enc);

#endif
