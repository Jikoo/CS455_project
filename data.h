#ifndef DATA_H
#define DATA_H 1

#include <dirent.h>

#define MAX_NOTES 99

int list_notes(const char *folder_name);

int is_note(char *file_name);

// Get a file name from user input.
// Allocates memory to store result.
// Returns `NULL` on error, printing issues to `stderr`.
char* intake_file_name(unsigned long *len_ptr);

int next_file_name(const char *folder_name);

void add_notes_in_folder(const unsigned char *key, const char *folder_name, const char *input);

void decrypt_note(const unsigned char *key, const char *folder_name, const char *input);

#endif
