#include "security.h"
#include <openssl/evp.h>
#include <openssl/sha.h>

// Reqs -lcrypto (and maybe -lssl depending on OS) flags to link openssl
// Might need -lbsd for arc4rand depending on OS

void generate_salt(unsigned char ptr[8]) {
  // Vulnerability resolution: secure random number generation used.
  // https://man.openbsd.org/cgi-bin/man.cgi/OpenBSD-current/man3/arc4random.3
  arc4random_buf(ptr, 8);
}

unsigned char* calculate_hash(char *input, unsigned char salt[8]) {
  // SHA-256 from OpenSSL
  // https://github.com/falk-werner/openssl-example/blob/main/doc/sha256.md
  // Initialize context.
  EVP_MD_CTX *context = EVP_MD_CTX_create();
  EVP_DigestInit(context, EVP_sha256());

  // This is actually unnecessary complexity to introduce a "vulnerability"
  // EVP_DigestUpdate can just be called subsequently on both pieces of data.
  unsigned long len1 = strlen(input);
  unsigned long total = 0;

  // Vulnerability: Overflow protection
  // Could add a pure math check for signed if we'd prefer
  if (__builtin_add_overflow(len1, 8, &total)) {
    return 0;
  }

  unsigned char *ptr = malloc(total);
  // Like strncpy, but won't complain about typing and won't stop on \0 in salt.
  memcpy(ptr, input, len1);
  // TODO this is probably wrong
  memcpy(ptr + len1 * sizeof(char), salt, 8);

  // Add data.
  EVP_DigestUpdate(context, ptr, total);

  // Vulnerability resolution: No memory leak; memory is freed.
  free(ptr);

  // Produce finalized result.
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int result_length = 0;
  EVP_DigestFinal(context, result, &result_length);

  // Free residual context data.
  EVP_MD_CTX_destroy(context);

  if (result_length != SHA256_DIGEST_LENGTH) {
    fprintf(stderr, "Got unexpected length %d for SHA256 (%d)", result_length, SHA256_DIGEST_LENGTH);
    return 0;
  }

  // Vulnerability: Memory allocation -> leak.
  // We could require a fixed SHA256_DIGEST_LENGTH array pointer passed in instead, but that would be less spicy.
  ptr = malloc(result_length);
  memcpy(ptr, result, result_length);
  return ptr;
}

int log_in(char *password, unsigned char salt[8], unsigned char hash[SHA256_DIGEST_LENGTH]) {
  unsigned char *calculated = calculate_hash(password, salt);

  // If hash is unexpected length, deny attempt.
  if (calculated <= 0) {
    return 0;
  }

  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    if (calculated[i] != hash[i]) {
      return 0;
    }
  }

  // Vulnerability resolution: No memory leak; memory is freed.
  free(calculated);

  return 1;
}

void cipher(unsigned char *in, int len, FILE *out, unsigned char *key, unsigned char *iv, int enc) {
  // Set up cipher context for AES-256 CBC encoding/decoding.
  EVP_CIPHER_CTX *context;
  EVP_CipherInit(context, EVP_aes_256_cbc(), key, iv, enc);

  int result_len;
  unsigned char *result = malloc(2 * len);
  // Perform operation and write to file.
  EVP_CipherUpdate(context, result, &result_len, in, len);
  fwrite(result, sizeof(char), result_len, out);

  // Finalize operation and write rest to file.
  EVP_CipherFinal(context, result, &result_len);
  fwrite(result, sizeof(char), result_len, out);

  // Free residual context data.
  EVP_CIPHER_CTX_cleanup(context);
}
