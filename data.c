#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "security.h"
#include "data.h"

int is_note(char *file_name) {
  // If the file name doesn't start with . and a digit, it's not a note.
  if (file_name[0] != '.' || !isdigit(file_name[1])) {
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

int list_notes(const char *folder_name) {
  DIR *dir = opendir(folder_name);
  if (dir <= 0) {
    // If the directory exists, print errors. Otherwise ignore.
    if (errno != ENOENT) {
      perror(folder_name);
    }
    return 0;
  }

  struct winsize wsize;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);

  int width = wsize.ws_col;
  int cols = (width - 6) / 8 + 2;

  int count = 0;
  struct dirent *entry;
  while ((entry = readdir(dir))) {
    if (is_note(entry->d_name)) {
      printf("%-6s", entry->d_name + sizeof(char));
      ++count;
      if (count % cols == 0) {
        printf("\n");
      } else {
        printf("  ");
      }
    }
  }

  // If we aren't at a clean line ending, add a newline anyway.
  if (count % cols != 0) {
    printf("\n");
  }

  closedir(dir);

  return count;
}

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

  if (*len_ptr <= 1) {
    fprintf(stderr, "Invalid file name '%s'! File names are numeric.\n", line);
    return NULL;
  }

  char *note_name = malloc(*len_ptr + 1);
  note_name[0] = '.';
  strncpy(note_name + sizeof(char), line, *len_ptr);

  free(line);

  return note_name;
}

int compare_names(const void *val1, const void *val2) {
  return strncmp(val1, val2, MAXNAMLEN);
}

int next_file_name(const char *folder_name) {
  DIR *dir = opendir(folder_name);
  if (dir <= 0) {
    perror(folder_name);
    return -1;
  }

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

  if (closedir(dir)) {
    perror(folder_name);
  }

  qsort(file_names, index, MAXNAMLEN, compare_names);

  char buf[MAXNAMLEN];
  for (int i = 0; i < index; ++i) {
    sprintf(buf, ".%d", i);
    // If the sorted index doesn't match, the number is not in use.
    // TODO edge case where .000 etc. cause problems
    // TODO numeric issues
    if (!strncmp(buf, file_names[i], MAXNAMLEN)) {
      return i;
    }
  }

  if (index < MAX_NOTES) {
    return index;
  }

  return -1;
}

void combined_path(const char *dir, const char *entry_name, char *result) {
  // Vulnerability: strcpy
  strcpy(result, dir);
  if (dir[strlen(dir) - 1] != '/') {
    strcat(result, "/");
  }
  // Vulnerability: strcat
  strcat(result, entry_name);
}

void add_notes_in_folder(const unsigned char *key, const char *folder_name, const char *input) {

  struct stat st;
  if (stat(folder_name, &st) <= 0) {
    // Create folder with read-write-execute permissions for the user
    mkdir(folder_name, S_IRUSR | S_IWUSR | S_IXUSR);
  }

  unsigned char iv[IV_SIZE];
  generate_iv(iv);

  int next_file_num = next_file_name(folder_name);
  char note_name[MAXNAMLEN];
  sprintf(note_name, ".%d", next_file_num);

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
  umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  FILE *noteBook = fopen(file_path, "w");
  if (!noteBook) {
    perror(file_path);
    return;
  }

  if (!cipher((unsigned char *)input, strlen(input), noteBook, key, iv, 1)) {
    fprintf(stderr, "Encryption failed.\n");
    fclose(noteBook);
    return;
  }

  if (fclose(noteBook)) {
    perror(note_name);
  }

  printf("Encrypted as note %s!", note_name);
}

void decrypt_note(const unsigned char *key, const char *folder_name, const char *note_name) {

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

  struct stat stat_val;
  if (stat(file_path, &stat_val) <= 0) {
    perror(file_path);
    return;
  }

  int fd = open(note_name, O_RDONLY);

  // TODO verify stat results!
  unsigned long file_len = stat_val.st_size;

  if (fd <= 0) {
    perror(note_name);
    return;
  }
  if (file_len < IV_SIZE * 2) {
    fprintf(stderr, "Note %s corrupted. Please delete.", note_name);
    return;
  }

  // Read IV.
  unsigned char iv[IV_SIZE];
  int bytes_read = read(fd, iv, IV_SIZE);
  if (bytes_read < IV_SIZE) {
    perror(note_name);
    return;
  }

  // Read encrypted content.
  unsigned char* contents = malloc(file_len - 32);
  bytes_read = read(fd, contents, file_len);
  // TODO verify length
  // TODO check new output value
  cipher(contents, bytes_read, stdout, key, iv, 0);

}
