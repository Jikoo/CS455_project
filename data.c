// Authors: Adam and Alex
// Resources used:
// Adam's labs from CS-355

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "security.h"
#include "data.h"

// Check if a file name is a note name.
//
// `file_name`: the name to check
// Author: Adam
int is_note(char *file_name) {
  // If the file name doesn't start with . and a non-zero digit, it's not a note.
  // Checking index 1 is safe because index 0 must be '.', meaning at worst 1 is '\0'.
  if (file_name[0] != '.' || file_name[1] < '1' || '9' < file_name[1]) {
    return 0;
  }

  // Allow any number of additional digits.
  for (int i = 2; i <= MAXNAMLEN; ++i) {
    if (file_name[i] == '\0') {
      return 1;
    }
    if (!isdigit(file_name[i])) {
      return 0;
    }
  }

  // If the file is the max name length and every character matched, it's okay.
  // Weird though, because unsigned longs don't have 255 digits in base 10.
  return 1;
}

// List notes in a folder.
//
// `folder_name`: path of directory containing note files
// Author: Adam, Alex (merged two different versions)
int list_notes(const char *folder_name) {
  DIR *dir = opendir(folder_name);
  if (dir <= 0) {
    // If the directory exists, print errors. Otherwise ignore.
    if (errno != ENOENT) {
      perror(folder_name);
    }
    return 0;
  }

  // Get window width to determine max entries displayable in a row.
  struct winsize wsize;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);

  int width = wsize.ws_col;
  // Initial column + remainder / characters per column
  int cols = 1 + (width - 6) / 8;

  int count = 0;
  struct dirent *entry;
  while ((entry = readdir(dir))) {
    if (is_note(entry->d_name)) {
      // Assume that the average file name will not exceed 6 characters.
      // Our own limit is 3 because MAX_NOTES is 99.
      printf("%-6s", entry->d_name + sizeof(char));
      ++count;
      if (count % cols == 0) {
        // End of row, next row.
        printf("\n");
      } else {
        // Space out next entry so it is clearly separate.
        printf("  ");
      }
    }
  }

  // If we aren't at a clean line ending, add a newline anyway.
  if (count % cols != 0) {
    printf("\n");
  }

  // Clean up and finish.
  closedir(dir);

  return count;
}

// Get a file name from user input.
// Note: Allocates memory to store result.
// Returns `NULL` on error, printing issues.
//
// `len_ptr`: A pointer that will be filled with the accepted input length
// Author: Adam
char* intake_file_name(unsigned long *len_ptr) {
  char *line = NULL;
  int read = getline(&line, len_ptr, stdin);

  // Handle errors getting input.
  if (read <= 0) {
    perror("filename");
    return NULL;
  }

  // Truncate input to the first invalid character.
  for (int i = 0; i < read; ++i) {
    if (!isdigit(line[i])) {
      line[i] = '\0';
      *len_ptr = i + 1;
      break;
    }
  }

  // If the input does not start with a number, complain.
  if (*len_ptr <= 1) {
    printf("Invalid file name! File names are numeric.\n");
    free(line);
    return NULL;
  }

  // Otherwise, allocate memory to store just what we need of it.
  char *note_name = malloc(*len_ptr + 1);

  if (note_name == NULL) {
    perror("note name");
    free(line);
    return NULL;
  }

  // .<input>
  note_name[0] = '.';
  strncpy(note_name + sizeof(char), line, *len_ptr);

  // Free original input allocation.
  free(line);

  return note_name;
}

// Compare note names.
// Note that this only returns valid results for names that succeed `is_note(char *file_name)`!
int compare_names(const void *val1, const void *val2) {
  int len1 = strlen(val1);
  int len2 = strlen(val2);

  // If one of the strings is longer, it is a number with more digits.
  // Because we don't allow preceding 0s, the number with the most digits is the highest.
  if (len1 < len2) {
    return -1;
  }
  if (len1 > len2) {
    return 1;
  }

  // Otherwise, the strings have the same length. Compare character codepoints.
  return strncmp(val1, val2, len1);
}

// Find the next unused file name number.
// File names are always numeric to prevent information leakage via titles.
// Returns the next file number or `0` if files `1` through `MAX_NOTES` exist.
//
// `folder_name`: path of directory containing note files
// Author: Adam
int next_file_name(const char *folder_name) {
  DIR *dir = opendir(folder_name);
  if (dir <= 0) {
    perror(folder_name);
    return -1;
  }

  // Read up to MAX_NOTES of note names from note directory.
  char file_names[MAX_NOTES][MAXNAMLEN];
  int index = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) > 0 && index < MAX_NOTES) {
    if (!is_note(entry->d_name)) {
      continue;
    }
    strncpy(file_names[index], entry->d_name, MAXNAMLEN);
    ++index;
  }

  // Warn about issues closing directory, if any.
  if (closedir(dir)) {
    perror(folder_name);
  }

  // If there are no files, this is the first.
  if (index == 0) {
    return 1;
  }

  // Quicksort file names with our note name comparator.
  qsort(file_names, index, MAXNAMLEN, compare_names);

  // Iterate over numbers 1 - index and check for mismatches.
  // The first mismatch is an available file number.
  char buf[MAXNAMLEN];
  buf[0] = '.';
  for (int i = 1; i <= index; ++i) {
    sprintf(buf + sizeof(char), "%d", i);
    if (strncmp(buf, file_names[i - 1], MAXNAMLEN)) {
      return i;
    }
  }

  // If there were no mismatches, the file number is the next free file number.
  if (index < MAX_NOTES) {
    return index + 1;
  }

  // If there are no free file numbers, indicate that.
  return 0;
}

// Combine a directory and a file name into a file path.
// Note: This introduces vulnerabilities! Check directory and file name lengths before using!
//
// `dir`: the path of the base directory
// `entry_name`: the name of the file in the directory
// `result`: A pointer to where the result is to be placed
// Author: Adam
void combined_path(const char *dir, const char *entry_name, char *result) {
  // Vulnerability: strcpy
  strcpy(result, dir);
  if (dir[strlen(dir) - 1] != '/') {
    strcat(result, "/");
  }
  // Vulnerability: strcat
  strcat(result, entry_name);
}

int is_symlink(const char *path) {
    struct stat path_stat;

    if (lstat(path, &path_stat) != 0) {
        perror("lstat");
        return 0; // error
    }

    // Check it's a directory
    if (!S_ISDIR(path_stat.st_mode)) {
        return 0; // not a directory
    }

    // Check it's NOT a symlink
    if (S_ISLNK(path_stat.st_mode)) {
        return 0; // is a symlink
    }

    return 1; // it's a real directory!
}

// Encrypt and save a new note.
//
// `key`: the key to use for encryption
// `folder_name`: path of directory containing note files
// `input`: the plaintext note content
// Author: Alex
void add_note(const unsigned char *key, const char *folder_name, const char *input) {

  // Ensure that folder exists.
  struct stat st;
  if (stat(folder_name, &st) <= 0) {
    // Create folder with read-write-execute permissions for the user
    mkdir(folder_name, S_IRUSR | S_IWUSR | S_IXUSR);
  }

  // Get the next file number.
  int next_file_num = next_file_name(folder_name);

  // If no file number is available, finish.
  if (!next_file_num) {
    printf("Too many notes present to create another!\n");
    printf("Only up to %d notes are supported.\n", MAX_NOTES);
    return;
  }

  // Set up note name.
  char note_name[MAXNAMLEN];
  sprintf(note_name, ".%d", next_file_num);

  // Resolve combined_path vulnerabilities by checking lengths before calls.
  int len1 = strlen(folder_name);
  int len2 = strlen(note_name);
  int total = 0;
  if (__builtin_add_overflow(len1, len2, &total)
      || __builtin_add_overflow(total, 1, &total)
      || total > PATH_MAX) {
    fprintf(stderr, "Path too long: %s/%s", folder_name, note_name);
    return;
  }
  char file_path[PATH_MAX];
  combined_path(folder_name, note_name, file_path);

  // Make file accessible only by user.
  __mode_t existing_mask = umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  FILE *noteBook = fopen(file_path, "w");
  // Reset umask after file creation.
  umask(existing_mask);

  // If file cannot be opened, print.
  if (!noteBook) {
    perror(file_path);
    return;
  }

  // Generate a new IV for this file.
  unsigned char iv[IV_SIZE];
  generate_iv(iv);

  // Write the IV to the top of the file, stopping on error.
  if (fwrite(iv, sizeof(unsigned char), IV_SIZE, noteBook) != IV_SIZE) {
    perror(file_path);
    fclose(noteBook);
    return;
  }

  // Encrypt the input.
  if (!cipher((unsigned char *)input, strlen(input), noteBook, key, iv, 1)) {
    printf("Encryption as note %s failed.\n", note_name + sizeof(char));
    fclose(noteBook);
    return;
  }

  // Close file and warn if closing fails.
  if (fclose(noteBook)) {
    perror(note_name);
  }

  printf("Encrypted as note %s!", note_name + sizeof(char));
}

// Decrypt and print a new note.
//
// `key`: the key to use for decryption
// `folder_name`: path of directory containing note files
// `input`: the name of the note file
// Author: Adam
void read_note(const unsigned char *key, const char *folder_name, const char *note_name) {
  // Resolve combined_path vulnerabilities by checking lengths before calls.
  int len1 = strlen(folder_name);
  int len2 = strlen(note_name);
  int total = 0;
  if (__builtin_add_overflow(len1, len2, &total)
      || __builtin_add_overflow(total, 1, &total)
      || total > PATH_MAX) {
    fprintf(stderr, "Path too long: %s/%s", folder_name, note_name);
    return;
  }
  char file_path[PATH_MAX];
  combined_path(folder_name, note_name, file_path);

  // Get lstat of file.
  struct stat lstat_val;
  if (lstat(file_path, &lstat_val)) {
    perror(file_path);
    return;
  }

  // Open file.
  int fd = open(file_path, O_RDONLY);

  // Handle failure to open file.
  if (fd <= 0) {
    perror(file_path);
    return;
  }

  // Get fstat of file.
  struct stat fstat_val;
  if (fstat(fd, &fstat_val)) {
    perror(file_path);
    return;
  }

  // Ensure that file is the intended target file.
  if (lstat_val.st_ino != fstat_val.st_ino) {
    fprintf(stderr, "File %s was moved while opening!", file_path);
    return;
  }

  // Get file size from stat data.
  unsigned long file_len = fstat_val.st_size;
  if (file_len < IV_SIZE * 2) {
    fprintf(stderr, "Note %s corrupted. Please delete %s\n", note_name + sizeof(char), file_path);
    return;
  }

  // Read IV.
  unsigned char iv[IV_SIZE];
  int bytes_read = read(fd, iv, IV_SIZE);
  if (bytes_read < IV_SIZE) {
    fprintf(stderr, "Only read %d of IV\n", bytes_read);
    return;
  }

  // Remove IV from remaining content.
  file_len -= IV_SIZE;

  // Read encrypted content.
  unsigned char* contents = malloc(file_len);
  bytes_read = read(fd, contents, file_len);

  // If unable to read fully, warn about possible data loss.
  if (bytes_read != file_len) {
    printf("Unable to read note fully! File may be corrupted.\n");
    printf("Consider creating a new note with any recovered contents and deleting %s.\n", note_name + sizeof(char));
  }

  // Decrypt contents and print.
  cipher(contents, bytes_read, stdout, key, iv, 0);

}
