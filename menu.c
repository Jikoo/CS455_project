#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include "security.h"
#include "data.h"

#define MAX_INPUT_SIZE 100

const char *folder = "My Notebook";
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

int main_menu(unsigned char *secret);

void view_menu(unsigned char *secret);

void add_menu(unsigned char *secret);

void delete_menu();

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
  printf("\nWelcome to the Menu System!\n");
  printf("------------------------------------------------------------------------------------------\n");
  printf("First time setup detected.\n");
  printf("By entering a password, you will initialize the secure datastore.\n");
  printf("Make sure to remember this password, as it will be required to access data in the future.\n");
  printf("------------------------------------------------------------------------------------------\n");

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
    if (read < 0) {
      perror("password");
      free(pwd);
      return 1;
    }

    // Turn echo back on.
    reset_termios();

    printf("\n");

    // Error handling for input failure.
    if (pwd <= 0) {
      perror("password");
      free(pwd);
      return 1;
    }
  }

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
        fprintf(stderr, "Login details file (%s) is corrupted! Please delete it.", login_storage);
      }
      return 1;
    }
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
    fd = open(login_storage, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    if (fd <= 0) {
      perror(login_storage);
      return 1;
    }

    // Save to disk.
    if (write(fd, &details, details_len) != details_len) {
      perror(login_storage);
      return 1;
    }
  }

  unsigned char *secret = log_in(pwd, details.salt, details.hash);

  if (pwFromPrompt) {
    free(pwd);
  }

  if (secret != NULL) {
    while (main_menu(secret)) {
      // While exit is not selected, always re-enter main menu after completion.
    }
    printf("\nHave a super day!\n");
  } else {
    printf("Access DENIED. Get outta here, you rascal.\n");
    sleep(1);
  }

  free(secret);
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
  //int count = list_notes();
  int count = list_notes(folder);

  if (count <= 0) {
    printf("\nNothing to view!\n");
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

  printf("Decrypting note %s!", note_name + sizeof(char));
  decrypt_note(secret, folder, note_name);

  free(note_name);
  pause_for_input();
}

void add_menu(unsigned char *secret) {
  printf("Please enter the note's content:\n");

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

  // Encrypt to file
  input[strcspn(input, "\n")] = 0;
  add_notes_in_folder(secret, folder, input);
  pause_for_input();
}

void delete_menu() {
  printf("Current notes:\n");
  int count = list_notes(folder);

  if (count <= 0) {
    printf("\nNothing to delete!\n");
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

  // TODO delete

  printf("Woah, you selected note %s!", note_name + sizeof(char));

  pause_for_input();
}
