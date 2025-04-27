#ifndef SECURITY_H
#define SECURITY_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/types.h>

// 64-bit salt size in bytes.
#define SALT_SIZE 8
// 256-bit AES-256 key size in bytes.
#define KEY_SIZE 32
// 128-bit CBC mode AES-256 IV in bytes.
#define IV_SIZE 16

// Generate a new random salt.
//
// `buf`: buffer in which to place the new salt
void generate_salt(unsigned char buf[SALT_SIZE]);

// Generate a new random IV.
//
// `buf`: buffer in which to place the new IV
void generate_iv(unsigned char buf[IV_SIZE]);

// Calculate a SHA-256 hash of the given input.
//
// `input`: The input value
// `salt`: The salt to append to the input
unsigned char* calculate_hash(const char *input, const unsigned char salt[SALT_SIZE]);

// Authenticate using a password.
//
// `password`: The user password
// `salt`: The salt to append to the password
// `hash`: The expected hash
unsigned char* log_in(const char *password, const unsigned char salt[SALT_SIZE], const unsigned char hash[SHA256_DIGEST_LENGTH]);

// Encrypt or decrypt using the AES-256 algorithm.
//
// `in`: The content to encrypt or decrypt
// `len`: The input length
// `out`: The `FILE` to write to
// `key`: The AES-256 key
// `iv`: The IV used for CBC mode AES-256
int cipher(const unsigned char *in, const int len, FILE *out, const unsigned char key[KEY_SIZE], const unsigned char iv[IV_SIZE], int enc);

#endif
