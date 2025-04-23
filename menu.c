#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

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
  // Turn off echo and awaiting newline for inputs.
  instant_no_echo = originalt;
  instant_no_echo.c_lflag &= ~(ICANON | ECHO);

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

  // TODO check if secure store exists, prompt acc creation

  int pwFromPrompt = 0;
  if (*pwd == 0) {
    // TODO intake password
    *pwd = getline(NULL, NULL, stdin);
    // TODO error handling
  }

  // TODO authenticate (if not creating account?)

  // TODO should secret still be pwd?

  while (main_menu(pwd)) {
    // While exit is not selected, always re-enter main menu after completion.
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
