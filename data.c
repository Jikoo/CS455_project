#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "security.h"
#include "data.h"

#define KEY_LENGTH 32  // AES-256 = 32 bytes
#define IV_LENGTH 16   // AES block size

int is_note(char* filename) {
  if (filename[0] != '.' || !isdigit(filename[1])) {
    return 0;
  }
  for(int i = 2; i <= MAXNAMLEN; ++i) {
    if (filename[i] == '\0') {
      return 1;
    }
    if (!isdigit(filename[i])) {
      return 0;
    }
  }
  return 1;
}

int list_notes() {
  DIR *dir = opendir(".");
  if (dir <= 0) {
    perror(".");
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
  return count;
}

int list_notes_in_folder(const char *folder_name) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(folder_name);
    if (dir == NULL) {
        perror("Could not open folder");
        return 0;
    }

    struct winsize wsize;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);

    int width = wsize.ws_col;
    int cols = (width - 6) / 8 + 2;


    int index = 1;

    printf("Notebook '%s':\n", folder_name);
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        printf("Note %d) %s\n", index, entry->d_name);
        index++;
    }

    closedir(dir);

    return index;
}

void add_notes_in_folder(const char *folder_name, const char *input) {
    
  unsigned char key[KEY_LENGTH];
  unsigned char iv[IV_LENGTH];

  if (!generate_key_iv(key, iv)) {
    fprintf(stderr, "Failed to generate key and IV\n");
    pause_for_input();
    return;
  }
    char filename[256];

    struct stat st = {0};
    if (stat(folder_name, &st) == -1) {
        mkdir(folder_name, 0700);  // Create folder with read-write-execute permissions for the user
    }

    filename[strcspn(filename, "\n")] = 0; // Remove trailing newline if present

    printf("Enter filename to save the encrypted note: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        fprintf(stderr, "Failed to read filename.\n");
        pause_for_input();
        return;
    }

    char filePath[512];
    snprintf(filePath, sizeof(filePath), "%s/%s", folder_name, filename);
    FILE *noteBook = fopen(filePath, "wb");
    if (!noteBook) {
        perror("Failed to open file for writing");
        pause_for_input();
        return;
    }

    if (!cipher((unsigned char *)input, strlen(input), noteBook, key, iv, 1)) {
        fprintf(stderr, "Encryption failed.\n");
        fclose(noteBook);
        pause_for_input();
        return;
    }

    fclose(noteBook);

    printf("Encrypted as note %s!", filename);
}

char *getNoteName(const char *folder_name, int note_number){
  
  int index = 0;

  DIR *dir;
  struct dirent *entry;

  dir = opendir(folder_name);
  if (dir == NULL) {
      perror("Could not open folder");
      return 0;
  }

  while ((entry = readdir(dir)) != NULL) {
    if(index == note_number){
       char *noteName = malloc(strlen(entry->d_name) + 1);
        if (noteName == NULL) {
          perror("Failed to allocate memory");
          closedir(dir);
          return NULL;
        }
        strcpy(noteName, entry->d_name);
        printf("\nOpening %s\n", noteName);
        closedir(dir);
        return noteName;
    }
    else{
      index++;
    }
  }

  return NULL;
}

void decrypt_note(const char *folder_name, int note_number) {

    char *filename = getNoteName(folder_name, note_number);
    char filePath[512];
    snprintf(filePath, sizeof(filePath), "%s/%s", folder_name, filename);
    FILE *noteBook = fopen(filePath, "rb");

    if (!noteBook) {
        perror("Error opening file");
        return;
    }

    unsigned char key[KEY_LENGTH];
    unsigned char iv[IV_LENGTH];
    if (!generate_key_iv(key, iv)) {
        fclose(noteBook);
        return;
    }

    int current_note_number;
    unsigned char decrypted_note[256];
    int note_found = 0;

    while (fread(&current_note_number, sizeof(current_note_number), 1, noteBook) == 1) {
        if (current_note_number == note_number) {
            // Decrypt the note content
            int len;
            len = fread(decrypted_note, sizeof(unsigned char), sizeof(decrypted_note), noteBook);
            decrypted_note[len] = '\0';  // Null terminate the string

            if (!cipher(decrypted_note, len, NULL, key, iv, 0)) {
                fprintf(stderr, "Decryption failed.\n");
                fclose(noteBook);
                return;
            }

            printf("Decrypted Note #%d: %s\n", note_number, decrypted_note);
            note_found = 1;
            break;
        } else {
            // Skip this note content if the number doesn't match
            fseek(noteBook, sizeof(decrypted_note), SEEK_CUR);
        }
    }

    if (!note_found) {
        printf("Note #%d not found.\n", note_number);
    }

    fclose(noteBook);
}