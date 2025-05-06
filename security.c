// Authors: Adam and Alex
// Resources used:
// https://man.openbsd.org/cgi-bin/man.cgi/OpenBSD-current/man3/arc4random.3
// https://github.com/falk-werner/openssl-example/blob/main/doc/sha256.md
// https://stackoverflow.com/a/24899425

#include "security.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

// Generate a new random salt.
//
// `buf`: buffer in which to place the new salt
// Author: Alex, Adam (merged implementations)
void generate_salt(unsigned char buf[SALT_SIZE]) {
  // Prefer OpenSSL random.
  if (RAND_bytes(buf, SALT_SIZE) != 1) {
    ERR_print_errors_fp(stderr);
    // Fall back to arc4rand if OpenSSL fails.
    arc4random_buf(buf, SALT_SIZE);
  }
}

// Generate a new random IV.
//
// `buf`: buffer in which to place the new IV
// Author: Alex, Adam (merged implementations)
void generate_iv(unsigned char buf[IV_SIZE]) {
  // Prefer OpenSSL random.
  if (RAND_bytes(buf, IV_SIZE) != 1) {
    ERR_print_errors_fp(stderr);
    // Fall back to arc4rand if OpenSSL fails.
    arc4random_buf(buf, IV_SIZE);
  }
}

// Calculate a SHA-256 hash of the given input.
// Returns a hash of the input with the salt appended.
// Note: This allocates memory to contain the resulting hash!
//
// `input`: The input value
// `salt`: The salt to append to the input
// Author: Adam
unsigned char* calculate_hash(const char *input, const unsigned char salt[SALT_SIZE]) {
  // Create new message digest context.
  EVP_MD_CTX *context = EVP_MD_CTX_new();
  if (context == NULL) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // Initialize message digest context.
  if (!EVP_DigestInit_ex2(context, EVP_sha256(), NULL)) {
    EVP_MD_CTX_free(context);
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // This is actually unnecessary complexity to introduce a "vulnerability"
  // EVP_DigestUpdate can just be called subsequently on both pieces of data.
  unsigned long len1 = strlen(input);
  unsigned long total = 0;

  // Vulnerability: Overflow protection
  // Could add a pure math check for signed if we'd prefer
  if (__builtin_add_overflow(len1, SALT_SIZE, &total)) {
    EVP_MD_CTX_free(context);
    return NULL;
  }

  // Allocate memory for data.
  unsigned char *ptr = malloc(total);
  if (ptr == NULL) {
    perror("hash input");
    EVP_MD_CTX_free(context);
    return NULL;
  }

  // Like strncpy, but won't complain about typing and won't stop on \0 in salt.
  memcpy(ptr, input, len1);
  memcpy(ptr + len1 * sizeof(char), salt, SALT_SIZE);

  // Add data.
  if (!EVP_DigestUpdate(context, ptr, total)) {
    EVP_MD_CTX_free(context);
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // Vulnerability resolution: No memory leak; memory is freed.
  free(ptr);

  // Produce finalized result.
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int result_length = 0;
  if (!EVP_DigestFinal_ex(context, result, &result_length)) {
    EVP_MD_CTX_free(context);
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // Free residual context data.
  EVP_MD_CTX_free(context);

  if (result_length != SHA256_DIGEST_LENGTH) {
    fprintf(stderr, "Got unexpected length %d for SHA256 (%d)", result_length, SHA256_DIGEST_LENGTH);
    return NULL;
  }

  // Vulnerability: Memory allocation -> leak.
  // We could require a fixed SHA256_DIGEST_LENGTH array pointer passed in instead, but that would be less spicy.
  ptr = malloc(result_length);
  if (ptr == NULL) {
    return NULL;
  }

  memcpy(ptr, result, result_length);
  return ptr;
}

// Authenticate using a password.
// Returns a secret for use as a key in encryption and decryption.
// Note: This allocates memory to store the resulting secret!
//
// `password`: The user password
// `salt`: The salt to append to the password
// `hash`: The expected hash
// Author: Adam
unsigned char* log_in(const char *password, const unsigned char salt[SALT_SIZE], const unsigned char hash[SHA256_DIGEST_LENGTH]) {
  unsigned char *calculated = calculate_hash(password, salt);

  // If hash is not available, deny attempt.
  if (calculated == NULL) {
    return NULL;
  }

  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    if (calculated[i] != hash[i]) {
      return NULL;
    }
  }

  // Vulnerability resolution: No memory leak; memory is freed.
  free(calculated);

  // Hash actual password for use as secret.
  EVP_MD_CTX *context = EVP_MD_CTX_new();
  if (context == NULL) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  if (!EVP_DigestInit_ex2(context, EVP_sha256(), NULL)
    || !EVP_DigestUpdate(context, password, strlen(password))) {
    EVP_MD_CTX_free(context);
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // Allocate memory for result.
  unsigned char *result = malloc(EVP_MAX_MD_SIZE);
  if (result == NULL) {
    perror("hash secret");
    EVP_MD_CTX_free(context);
    return NULL;
  }

  // Finalize result.
  unsigned int result_length = 0;
  if (!EVP_DigestFinal_ex(context, result, &result_length)) {
    EVP_MD_CTX_free(context);
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // Free residual context data.
  EVP_MD_CTX_free(context);

  return result;
}

// Encrypt or decrypt using the AES-256 algorithm.
// Returns 1 on success, 0 otherwise.
//
// `in`: The content to encrypt or decrypt
// `len`: The input length
// `out`: The `FILE` to write to
// `key`: The AES-256 key
// `iv`: The IV used for CBC mode AES-256
// Author: Alex
int cipher(const unsigned char *in, const int len, FILE *out, const unsigned char key[KEY_SIZE], const unsigned char iv[IV_SIZE], int enc) {
  // Create new cipher context.
  EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new();
  if (!context) {
    ERR_print_errors_fp(stderr);
    return 0;
  }

  // Initialize cipher context.
  if (!EVP_CipherInit_ex(context, EVP_aes_256_cbc(), NULL, key, iv, enc)) {
    ERR_print_errors_fp(stderr);
    EVP_CIPHER_CTX_free(context);
    return 0;
  }

  // Allocate memory for result.
  int out_len;
  unsigned char *result = malloc(len + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
  if (result == NULL) {
    ERR_print_errors_fp(stderr);
    EVP_CIPHER_CTX_free(context);
    return 0;
  }

  // Update cipher with content.
  if (!EVP_CipherUpdate(context, result, &out_len, in, len)) {
    ERR_print_errors_fp(stderr);
    free(result);
    EVP_CIPHER_CTX_free(context);
    return 0;
  }

  // Write content to output.
  fwrite(result, 1, out_len, out);

  // Finalize cipher.
  int final_len;
  if (!EVP_CipherFinal_ex(context, result, &final_len)) {
    ERR_print_errors_fp(stderr);
    free(result);
    EVP_CIPHER_CTX_free(context);
    return 0;
  }

  // Write any finalized output.
  fwrite(result, 1, final_len, out);

  // Free up memory.
  free(result);
  EVP_CIPHER_CTX_free(context);

  return 1; // success
}
