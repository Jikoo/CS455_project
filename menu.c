#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "security.h"
#include "data.h"

// Define minimum password length.
#define MIN_PASSWORD_LEN 12

const char *folder = ".notebook";
const char *login_storage = ".login";

struct login_details {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  unsigned char salt[8];
};

static struct termios originalt;
static struct termios instant_no_echo;

// Turn echo off and don't wait for newlines.
void echo_icanon_off() {
  tcsetattr( STDIN_FILENO, TCSANOW, &instant_no_echo);
}

// Reset terminal to its original state.
// Turns echo back on and waits for newlines to process input.
void reset_termios() {
  tcsetattr(STDIN_FILENO, TCSANOW, &originalt);
}

// Await a keystroke from the user to avoid pushing content out of view.
void pause_for_input() {
  echo_icanon_off();
  printf("\nPress any key to return to the main menu...");
  getchar();
  reset_termios();
  printf("\n\n\n");
}

// Display the main menu.
int main_menu(unsigned char *secret);

// Display the "View Note" menu.
void view_menu(unsigned char *secret);

// Display the "Add Note" menu.
void add_menu(unsigned char *secret);

// Display the "Delete Note" menu.
void delete_menu();

// Accept a line of text as a password from the user.
int intake_password(char **pwd) {
  printf("Please enter your password: ");

  // Turn off echo. No password peeksies!
  struct termios no_echo = originalt;
  no_echo.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &no_echo);

  // Read line of input from user. Note that this allocates memory!
  *pwd = NULL;
  unsigned long pwd_len = 0;
  int read = getline(pwd, &pwd_len, stdin);

  if (read < 0 || pwd <= 0) {
    perror("password");
    return 1;
  }

  // Trim trailing newline.
  (*pwd)[strcspn(*pwd, "\n")] = '\0';

  // Turn echo back on.
  reset_termios();
  printf("\n");

  return 0;
}

int main(int argc, char *argv[]) {
  // Store the original terminal settings for later restoration.
  tcgetattr(STDIN_FILENO, &originalt);
  // Create a copy for modification.
  instant_no_echo = originalt;
  // Turn off echo and awaiting newline for inputs.
  instant_no_echo.c_lflag &= ~(ICANON | ECHO);

  // Check for cli password parameter.
  char *pwd = 0;
  char opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
      case 'p':
        pwd = optarg;
        break;
      default:
        continue;
    }
  }

  printf("\nWelcome to Secret Notes!\n");

  int pwd_allocated = 0;
  struct login_details details;
  int details_len = sizeof(details);

  // Try to read salt and hash from disk.
  int fd = open(login_storage, O_RDONLY);
  if (fd > 0) {
    // If the file was opened, we can read from it.
    int bytesRead = read(fd, &details, details_len);
    if (bytesRead != details_len) {
      if (bytesRead < 0) {
        perror(login_storage);
      } else {
        fprintf(stderr, "Login details file (%s) is corrupted! Please delete it.\n", login_storage);
      }
      return 1;
    }
    if (close(fd)) {
      perror(login_storage);
    }

    // If password is not specified, read it.
    if (pwd == 0) {
      // Flag for later memory deallocation.
      pwd_allocated = 1;
      intake_password(&pwd);
    }
  } else {
    // Otherwise, it probably doesn't exist. New account creation!
    printf("------------------------------------------------------------------------------------------\n");
    printf("First time setup detected.\n");
    printf("By entering a password, you will initialize the secure datastore.\n");
    printf("Make sure to remember this password, as it will be required to access data in the future.\n");
    printf("------------------------------------------------------------------------------------------\n");

    // If password is not specified, read it.
    if (pwd == 0 || strlen(pwd) < MIN_PASSWORD_LEN) {
      do {
        // Specify password requirements.
        printf("Passwords must be at least %d characters in length.\n", MIN_PASSWORD_LEN);
        printf("Any character you can type is supported, and the longer the password the better.\n");
        printf("Consider using a passphrase, like \"The quick brown fox jumped over the lazy dog!\"\n");
        printf("------------------------------------------------------------------------------------------\n");
        // Flag for later memory deallocation.
        pwd_allocated = 1;
        intake_password(&pwd);
      } while (strlen(pwd) < MIN_PASSWORD_LEN);
    }

    // Generate a new salt.
    generate_salt(details.salt);

    // Get the salted hash.
    unsigned char *hash = calculate_hash(pwd, details.salt);

    if (hash == 0) {
      if (pwd_allocated && &pwd > 0) {
        free(pwd);
      }
      return 1;
    }

    // Copy the hash into the login details.
    memcpy(details.hash, hash, SHA256_DIGEST_LENGTH);

    // Free up memory consumed by the hash.
    free(hash);

    // Open the login file.
    fd = open(login_storage, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    if (fd <= 0) {
      perror(login_storage);
      if (pwd_allocated && &pwd > 0) {
        free(pwd);
      }
      return 1;
    }

    // Save to disk.
    if (write(fd, &details, details_len) != details_len) {
      perror(login_storage);
      return 1;
    }
  }

  // Convert password to secret.
  unsigned char *secret = log_in(pwd, details.salt, details.hash);

  // Clean up password if possible.
  if (pwd_allocated && &pwd > 0) {
    free(pwd);
  }

  if (secret != NULL) {
    while (main_menu(secret)) {
      // While exit is not selected, always re-enter main menu after completion.
    }

    printf("\nGoodbye!\n");

    // Free memory allocated for secret.
    free(secret);
  } else {
    printf("Access denied. Make sure you have entered your password correctly.\n");
    sleep(1);
  }

  return 0;
}

int main_menu(unsigned char *secret) {
  printf("Please choose an option:\n");
  printf("  1) View note\n");
  printf("  2) Create note\n");
  printf("  3) Delete note\n");
  printf("  4) Exit\n");

  echo_icanon_off();

  char selection;
  while ((selection = getchar()) < '1' || selection > '4') {
    // printf("Invalid selection %c\n", selection);
  }

  reset_termios();

  switch (selection) {
    case '1':
      view_menu(secret);
      break;
    case '2':
      add_menu(secret);
      break;
    case '3':
      delete_menu(secret);
      break;
    case '4':
    default:
      return 0;
  }

  return 1;
}

void view_menu(unsigned char *secret) {
  printf("Current notes:\n");
  int count = list_notes(folder);

  if (count <= 0) {
    printf("No notes! Maybe you should write some.\n");
    pause_for_input();
    return;
  }

  printf("Which would you like to view?\n");

  unsigned long len = 0;
  char *note_name = intake_file_name(&len);

  if (note_name == NULL) {
    // Method handles error logging.
    free(note_name);
    return;
  }

  printf("Decrypting note %s!\n", note_name + sizeof(char));
  decrypt_note(secret, folder, note_name);

  free(note_name);
  pause_for_input();
}

void add_menu(unsigned char *secret) {
  printf("Please enter the note's content:\n");

  // Read line of input from user. Note that this allocates memory!
  char *input = NULL;
  unsigned long pwd_len = 0;
  int read = getline(&input, &pwd_len, stdin);

  if (read < 0 || input <= 0) {
    perror("content");
    return;
  }

  input[strcspn(input, "\n")] = 0;

  // Encrypt to file
  add_notes_in_folder(secret, folder, input);

  free(input);

  pause_for_input();
}

void delete_menu() {
  printf("Current notes:\n");
  int count = list_notes(folder);

  if (count <= 0) {
    printf("No notes! Maybe you should write some.\n");
    pause_for_input();
    return;
  }

  printf("Which would you like to delete?\n");

  unsigned long len = 0;
  char *note_name = intake_file_name(&len);

  if (note_name == NULL) {
    // Method handles error logging.
    return;
  }

  // Resolve combined_path vulnerabilities by checking lengths before calls.
  int len1 = strlen(folder);
  int len2 = strlen(note_name);
  int total = 0;
  if (__builtin_add_overflow(len1, len2, &total)
      || __builtin_add_overflow(total, 1, &total)
      || total > PATH_MAX) {
    fprintf(stderr, "Path too long: %s/%s", folder, note_name);
    return;
  }
  char file_path[PATH_MAX];
  combined_path(folder, note_name, file_path);

  printf("Deleting note %s.", note_name + sizeof(char));

  // Vulnerability mitigation: unlink rather than delete.
  // Filesystem will delete when links reach 0.
  if (unlink(file_path)) {
    perror(file_path);
  }

  free(note_name);
  pause_for_input();
}
