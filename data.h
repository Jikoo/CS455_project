#ifndef DATA_H
#define DATA_H

void echo_icanon_off();
void reset_termios();
int list_notes();
int is_note(char* filename);
int list_notes_in_folder(const char *folder_name);
void add_notes_in_folder(const char *folder_name, const char *input);
void pause_for_input();
void decrypt_note(const char *folder_name, int note_number);
char *getNoteName(const char *folder_name, int note_number);

#endif
