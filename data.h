#ifndef DATA_H
#define DATA_H 1

// Define max notes.
// This only comes into play when adding notes; extra notes on disk are supported.
#define MAX_NOTES 99

// List notes in a folder.
//
// `folder_name`: path of directory containing note files
int list_notes(const char *folder_name);

// Check if a file name is a note name.
//
// `file_name`: the name to check
int is_note(char *file_name);

// Get a file name from user input.
// Note: Allocates memory to store result.
// Returns `NULL` on error, printing issues.
//
// `len_ptr`: A pointer that will be filled with the accepted input length
char* intake_file_name(unsigned long *len_ptr);

// Find the next unused file name number.
// File names are always numeric to prevent information leakage via titles.
// Returns the next file number or `0` if files `1` through `MAX_NOTES` exist.
//
// `folder_name`: path of directory containing note files
int next_file_name(const char *folder_name);

// Combine a directory and a file name into a file path.
// Note: This introduces vulnerabilities! Check directory and file name lengths before using!
//
// `dir`: the path of the base directory
// `entry_name`: the name of the file in the directory
// `result`: A pointer to where the result is to be placed
void combined_path(const char *dir, const char *entry_name, char *result);

// Encrypt and save a new note.
//
// `key`: the key to use for encryption
// `folder_name`: path of directory containing note files
// `input`: the plaintext note content
void add_note(const unsigned char *key, const char *folder_name, const char *input);

// Decrypt and print a new note.
//
// `key`: the key to use for decryption
// `folder_name`: path of directory containing note files
// `input`: the name of the note file
void read_note(const unsigned char *key, const char *folder_name, const char *input);

#endif
