#define _XOPEN_SOURCE_EXTENDED // Must be first.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <ncursesw/curses.h> // Should be last.

void init() {
    // Set locale (so as to use UTF-8 characters).
    setlocale(LC_ALL, "en_US.UTF-8");

    // Set up screen.
    initscr();
    curs_set(FALSE);
    noecho();

    // Set up colors.
    use_default_colors();
    start_color();
    // Can now call init_pair(ID, foreground, background) to establish color pair IDs.
    // Would be useful for highlighting selected entry, for example.
}

void end() {
    endwin();
}

void take_input(char* id, char *buf, int n) {
    clear();
    box(stdscr, 0, 0);
    mvaddstr(0, 2, id);

    // TODO smaller box for input
    //  Since this uses ncursesw, can actually draw the box ourselves if necessary.
    move(5, 5);
    // Make cursor visible
    curs_set(TRUE);
    flushinp();
    nodelay(stdscr, FALSE);
    echo();

    // TODO vulnerability?
    getstr(buf)

    curs_set(false);
    noecho();
}

void menu_add() {
    clear();
    box(stdscr, 0, 0);
    mvaddstr(0, 2, "ADD PERSON");

    // TODO
}

// TODO persontype selection for add

void menu_remove() {
    clear();
    box(stdscr, 0, 0);
    mvaddstr(0, 2, "REMOVE PERSON");

    // TODO
}

void menu_edit() {
    clear();
    box(stdscr, 0, 0);
    mvaddstr(0, 2, "EDIT PERSON");

    // TODO
}

void menu_filter() {
    clear();
    box(stdscr, 0, 0);
    mvaddstr(0, 2, "SEARCH PEOPLE");

    // TODO
}

void main_menu() {
    noecho();
    curs_set(FALSE);

    clear();
    box(stdscr, 0, 0);
    mvaddstr(0, 2, "MAIN MENU");

    // Output menu.
    mvaddstr(3, 5, "1) Add a person");
    mvaddstr(4, 5, "2) Delete a person");
    mvaddstr(5, 5, "3) Edit a person");
    mvaddstr(6, 5, "2) List people");

    // Turn on nodelay
    nodelay(stdscr, TRUE);
    // Drop any existing inputs.
    flushinp();

    // TODO we could allow arrow keys,
    //  visible representation of selection,
    //  enter to confirm

    int pressed = getch();
    switch (pressed) {
        case '1':
            menu_add();
            break;
        case '2':
            menu_remove();
            break;
        case '3':
            menu_edit();
            break;
        default:
            menu_filter();
    }

}
