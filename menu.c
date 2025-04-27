// Resources used:
// Adam's labs from CS-355

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

// Constants for file locations.
const char *folder = ".notebook";
const char *login_storage = ".login";

// Struct for storing salted and hashed password and salt.
struct login_details {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  unsigned char salt[SALT_SIZE];
};

// Commonly-used terminal settings.
static struct termios originalt;
static struct termios instant_no_echo;

// Turn echo off and don't wait for newlines.
// Author: Adam
void echo_icanon_off() {
  tcsetattr( STDIN_FILENO, TCSANOW, &instant_no_echo);
}

// Reset terminal to its original state.
// Turns echo back on and waits for newlines to process input.
// Author: Adam
void reset_termios() {
  tcsetattr(STDIN_FILENO, TCSANOW, &originalt);
}

// Await a keystroke from the user to avoid pushing content out of view.
// Author: Adam
void pause_for_input() {
  echo_icanon_off();
  printf("\nPress any key to return to the main menu...");
  getchar();
  reset_termios();
  printf("\n\n\n");
}

// Display the main menu.
//
// `secret`: the key to use for encryption and decryption
int main_menu(unsigned char *secret);

// Display the "View Note" menu.
//
// `secret`: the key to use for encryption and decryption
void view_menu(unsigned char *secret);

// Display the "Add Note" menu.
//
// `secret`: the key to use for encryption and decryption
void add_menu(unsigned char *secret);

// Display the "Delete Note" menu.
void delete_menu();

// Accept a line of text as a password from the user.
// Side effects: Allocates memory to store password.
//
// Failure to read password will return 1, with the location being assigned to `NULL`.
// It is only necessary to check one of these conditions to properly handle errors.
//
// `pwd`: a pointer to the location of a char* that will hold the password.
// Author: Adam
int intake_password(char **pwd) {
  printf("Please enter your password: ");

  // Turn off echo. No password peeksies!
  struct termios no_echo = originalt;
  no_echo.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &no_echo);

  // Read line of input from user. Note that this allocates memory!
  *pwd = NULL;
  // This is a required parameter, but is set to define the length allocated, not exact string length!
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

// Main entry point. Parses command line parameters, reads in or sets up password
// information, intakes and verifies password, and starts main menu.
//
// `argc`: The number of arguments used when running the executable
// `argv`: The arguments used when running the executable
// Author: Adam
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
  } else {
    // For other errors (Access permission, file limit, etc.) log and exit.
    perror(login_storage);
    return 1;
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

// Display the main menu. Intakes a user selection from prompt and opens relevant submenu.
// Returns 0 when exiting, 1 otherwise.
//
// `secret`: the key to use for encryption and decryption
// Author: Alex
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

// Display the "View Note" menu. Displays existing notes, intakes a note name to view,
// and decrypts the note for display.
//
// `secret`: the key to use for encryption and decryption
// Author: Alex
void view_menu(unsigned char *secret) {
  // Print notes in notes directory.
  printf("Current notes:\n");
  int count = list_notes(folder);

  if (count <= 0) {
    printf("No notes! Maybe you should write some.\n");
    pause_for_input();
    return;
  }

  printf("Which would you like to view?\n");

  // Get file name.
  unsigned long len = 0;
  char *note_name = intake_file_name(&len);

  if (note_name == NULL) {
    // Method handles error logging.
    free(note_name);
    return;
  }

  // Decrypt note.
  printf("Decrypting note %s!\n", note_name + sizeof(char));
  read_note(secret, folder, note_name);

  // Free memory used by note name.
  free(note_name);

  // Pause so user can read before re-displaying main menu.
  pause_for_input();
}

// Display the "Add Note" menu. Reads a line of input, encrypts it, and saves to file.
//
// `secret`: the key to use for encryption and decryption
// Author: Alex
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

  // Encrypt to file.
  add_note(secret, folder, input);

  // Free memory used by input.
  free(input);

  // Pause so user can read before re-displaying main menu.
  pause_for_input();
}

// Display the "Delete Note" menu. Displays existing notes, intakes a note name to delete,
// and deletes the note.
//
// Author: Adam
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

  // Free memory used by input.
  free(note_name);

  // Pause so user can read before re-displaying main menu.
  pause_for_input();
}
