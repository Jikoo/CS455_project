#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

// TODO need to make a header file
// Should include IFDEF for handling reuse

// Reqs -lssl flag to link openssl
// Might need -lbsd for arc4rand

void generate_salt(unsigned char ptr[8]) {
  // Vulnerability resolution: secure random number generation used.
  // https://man.openbsd.org/cgi-bin/man.cgi/OpenBSD-current/man3/arc4random.3
  arc4random_buf(ptr, 8);
}

unsigned char* calculate_hash(unsigned char* input, int len) {
  // SHA-256 from OpenSSL
  // https://github.com/falk-werner/openssl-example/blob/main/doc/sha256.md
  // Initialize context.
  EVP_MD_CTX * context = EVP_MD_CTX_create();
  EVP_DigestInit(context, EVP_sha256());
  // Add data.
  EVP_DigestUpdate(context, input, len);
  // Produce finalized result.
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int result_length = 0;
  EVP_DigestFinal(context, result, &result_length);
  EVP_MD_CTX_free(context);

  if (result_length != SHA256_DIGEST_LENGTH) {
    fprintf(stderr, "Got unexpected length %d for SHA256 (%d)", result_length, SHA256_DIGEST_LENGTH);
    // TODO should this exit?
    return 0;
  }

  // Vulnerability: Memory allocation -> leak
  unsigned char* ptr = malloc(result_length);
  memcpy(ptr, result, result_length);
  return ptr;
}

int log_in(char* password, char* salt, unsigned char hash[SHA256_DIGEST_LENGTH]) {
  // TODO does this include \0?
  unsigned long len1 = strlen(password);
  unsigned long len2 = strlen(salt);
  unsigned long total = 0;
  
  // Vulnerability: Overflow protection
  // Could add a pure math check for signed if we'd prefer
  if (__builtin_add_overflow(len1, len2, &total)) {
    return 0;
  }

  unsigned char *ptr = malloc(total);

  // Vulnerability: memcpy? TODO look into this, is it actually one?
  memcpy(ptr, password, len1);
  memcpy(ptr + len1 * sizeof(char), salt, len2);

  unsigned char* calculated = calculate_hash(ptr, total);

  // Vulnerability resolution: No memory leak; memory is freed.
  free(ptr);

  for (int i = 0; i < total; ++i) {
    if (calculated[i] != hash[i]) {
      return 0;
    }
  }

  // Vulnerability resolution: No memory leak; memory is freed.
  free(calculated);

  return 1;
}

// TODO encrypt & decrypt

