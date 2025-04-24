#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "security.h"

struct login_details {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  unsigned char salt[8];
};

#define MAX_NOTES 10

static struct termios originalt;
static struct termios instant_no_echo;

int main_menu(char *secret);

void view_menu(char *secret);

void add_menu(char *secret);

void delete_menu(char *secret);

// TODO might want to move this to a separate file, data.c+h or something.
int list_notes(char *secret);

// Turn echo off and don't wait for newlines.
void echo_icanon_off() {
  tcsetattr( STDIN_FILENO, TCSANOW, &instant_no_echo);
}

// Reset termios: Turn echo back on, wait for newlines.
void reset_termios() {
  tcsetattr(STDIN_FILENO, TCSANOW, &originalt);
}

void pause_for_input() {
  echo_icanon_off();
  printf("\nPress any key to return to the main menu...");
  getchar();
  reset_termios();
  printf("\n\n\n");
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

  // TODO would be user-friendly to alert them that entering password will
  // create datastore on first time setup.

  int pwFromPrompt = 0;
  // If password is not specified, read it.
  if (pwd == 0) {
    // Flag for later memory deallocation.
    pwFromPrompt = 1;

    printf("Please enter your password: ");

    // Turn off echo. No password peeksies!
    struct termios no_echo = originalt;
    no_echo.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &no_echo);

    // Read line of input from user. Note that this allocates memory!
    unsigned long pwd_len = 0;
    int read = getline(&pwd, &pwd_len, stdin);

    // Turn echo back on.
    reset_termios();

    // Error handling for input failure.
    if (pwd <= 0) {
      perror("password");
      return 1;
    }
  }

  struct login_details details;
  int details_len = sizeof(details);

  // Try to read salt and hash from disk.
  int fd = open(".login", O_RDONLY);
  if (fd > 0) {
    // If the file was opened, we can read from it.
    int bytesRead = read(fd, &details, details_len);
  } else {
    // Otherwise, it probably doesn't exist. Generate a new salt.
    generate_salt(details.salt);

    // Get the salted hash.
    unsigned char *hash = calculate_hash(pwd, details.salt);

    if (hash == 0) {
      return 1;
    }

    // Copy the hash into the login details.
    memcpy(details.hash, hash, SHA256_DIGEST_LENGTH);

    // Free up memory consumed by the hash.
    free(hash);

    // Open the login file.
    fd = open(".login", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    if (fd <= 0) {
      perror(".login");
      return 1;
    }

    // Save to disk.
    if (write(fd, &details, details_len) != details_len) {
      perror(".login");
      return 1;
    }
  }

  if (log_in(pwd, details.salt, details.hash)) {
    // TODO should secret still be pwd?
    while (main_menu(pwd)) {
      // While exit is not selected, always re-enter main menu after completion.
    }
    printf("\nHave a super day!\n");
  } else {
    printf("Access DENIED. Get outta here, you rascal.\n");
    sleep(1);
  }

  if (pwFromPrompt) {
    free(pwd);
  }

  return 0;
}

int main_menu(char *secret) {
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

void view_menu(char *secret) {
  printf("Current notes:\n");
  int count = list_notes(secret);

  echo_icanon_off();

  if (count > 0) {
    printf("\nPlease select a note to view by number (0 - %d).\n", count - 1);

    char selection;
    while ((selection = getchar()) < '0' || selection > ('0' + count)) {
      // printf("Invalid selection %c\n", selection);
    }

    // TODO content
    printf("Woah, you selected note %c!", selection);
  }


  pause_for_input();
}

void add_menu(char *secret) {
  int count = 0; // TODO
  if (count >= MAX_NOTES) {
    printf("Oops, too many notes! You can only have %d!\n", MAX_NOTES);
    pause_for_input();
    return;
  }

  printf("Please enter the note's content:\n");

  // Just in case.
  reset_termios();

  // TODO may want to use getline instead (but would need to free when done!)
  char buf[256];
  char *input = fgets(buf, 256, stdin);

  if (input <= 0) {
    // errors
    printf("Oops, something went wrong!\n");
    if (input < 0) {
      perror("stdio");
    }
    pause_for_input();
    return;
  }


  // TODO write to file

  // TODO alert user
}

void delete_menu(char *secret) {
  printf("Current notes:\n");
  int count = list_notes(secret);

  echo_icanon_off();

  if (count <= 0) {
    pause_for_input();
    return;
  }

  printf("\nPlease select a note to delete by number (0 - %d).\n", count - 1);

  char selection;
  while ((selection = getchar()) < '0' || selection > ('0' + count)) {
    // printf("Invalid selection %c\n", selection);
  }

  // TODO content
  printf("Woah, you selected note %c!", selection);


  pause_for_input();
}

int list_notes(char *secret) {
  // TODO impl
  printf("No notes!");
  return 0;
}
