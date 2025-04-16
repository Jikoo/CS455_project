#include <stdio.h>
#include <string.h>

typedef enum {
    TEACHER,
    STAFF,
    STUDENT
} Role;

typedef enum {
    MATH,
    SCIENCE,
    ENGLISH,
    READING,
    WRITING
} Department;

struct TEACHER {
    int salary;
    Department department;
};

struct STUDENT {
    char grade;
};

typedef enum {
    SCHOOL_COUNSELOR,
    DEAN,
    PRINCIPAL,
    ASSISTANT_PRINCIPAL,
    JANITOR,
    LIBRARIAN,
    ATHLETICS_DIRECTOR
} Position;

struct STAFF {
    int salary;
    Position position;
};

typedef struct {
    char street[50];
    char city[50];
    char state[3]; // 2-Letter state code and null terminator
    char zip[10];
} Address;

typedef struct {
    char first_name[25];
    char middle_name[25];
    char last_name[25];
} Name;

typedef struct {
    int month;
    int day;
    int year;
} Date;

struct Person {
    Name name;
    Date date_of_birth;
    int age;
    char gender;
    int phone_number;
    char email[50];
    char password[30];
    Role role;
    Address address;
};

/* Write to file */

/* Function to display all */

/* Search Function */

/* Add to roster */
void writeToRoster(Role role) {
    FILE *filePointer;

    filePointer = fopen("schoolRoster.txt", "w");

    if (filePointer == NULL) {
        printf("File failed to open.");
    }

    struct Person entry;

    if (role == STUDENT) {
        entry.role = STUDENT;
    }
    if (role == TEACHER) {
        entry.role = TEACHER;
    }
    if (role == STAFF) {
        entry.role = STAFF;
    }

    printf("Please enter the first name: ");
    scanf("%s", entry.name.first_name);

    printf("Please enter the middle name: ");
    scanf("%s", entry.name.middle_name);

    printf("Please enter the last name: ");
    scanf("%s", entry.name.last_name);
}

/* Function to display options */
void displayOptions() {
    printf("1) Would you like to enter display mode? \n");
    printf("2) Would you like to enter search mode? \n");
    printf("3) Would you like to add, edit, or delete a student? \n");
    printf("4) Would you like to add, edit, or delete a teacher? \n");
    printf("5) Would you like to add, edit, or delete a staff member? \n");

    int selectedOption;
    char continueEdit = 'y';
    scanf("%d", &selectedOption);
    while (continueEdit == 'y') {
        switch (selectedOption) {
            case 1:
                printf("Entering display mode.\n");
                break;
            case 2:
                printf("Entering search mode.\n");
                break;
            case 3:
                printf("Entering student editor.\n");
                break;
            case 4:
                printf("Entering teacher editor.\n");
                break;
            case 5:
                printf("Entering staff editor.\n");
                break;
            default:
                printf("The option entered was invalid, please try again.\n");
                break;
        }

        /*printf("Would you like to continue? (y/n) ");
        scanf("%c", &continueEdit);

        while (continueEdit != 'y' || continueEdit != 'n') {
            printf("Your input was invalid, please try again. Enter 'y' or 'n'");
            scanf("%c", &continueEdit);
        }*/
    }
    
}


int main() {
    FILE *filePointer;

    filePointer = fopen("schoolRoster.txt", "w");

    if (filePointer == NULL) {
        printf("File failed to open.");
        return 1;
    }
    printf("Welcome to the school roster! Please Log in.\n");
    /*Log in function goes here*/

    char options;
    printf("Would you like to see the options? (y/n) ");
    scanf("%c", &options);

    if (options == 'y') {
        displayOptions();
    }

    

    fclose(filePointer);

    return 0;
}