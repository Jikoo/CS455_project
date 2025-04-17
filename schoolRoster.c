#include <stdio.h>
#include <string.h>
#include "dao.h"

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

    printf("Please enter the first name: ");
    scanf("%s", entry.name.first_name);

    printf("Please enter the middle name: ");
    scanf("%s", entry.name.middle_name);

    printf("Please enter the last name: ");
    scanf("%s", entry.name.last_name);

    /* Need to process these entries */
    printf("Please enter their month of birth (1-12): ");
    scanf("%d", &entry.date_of_birth.month);

    printf("Please enter their day of birth (1-31): ");
    scanf("%d", &entry.date_of_birth.day);

    printf("Please enter their year of birth: ");
    scanf("%d", &entry.date_of_birth.year);

    /* Should the age be calculated based on entry? */

    printf("Gender: ");
    scanf("%c", entry.gender);

    /* Phone number check? */
    printf("Phone number: ");
    scanf("%d", &entry.phone_number);

    printf("Email: ");
    scanf("%c", entry.email);

    printf("Street address: ");
    scanf("%c", entry.address.street);

    printf("Apartment number (If applicable): ");
    scanf("%c", entry.address.apartment);

    printf("City: ");
    scanf("%c", entry.address.city);

    /* Check for this? */
    printf("State (two letter code): ");
    scanf("%c", entry.address.state);


    if (role == STUDENT) {
        entry.role = STUDENT;
        
    }
    if (role == TEACHER) {
        entry.role = TEACHER;
    }
    if (role == STAFF) {
        entry.role = STAFF;
    }    
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
                writeToRoster( STUDENT);
                break;
            case 4:
                printf("Entering teacher editor.\n");
                writeToRoster(TEACHER);
                break;
            case 5:
                printf("Entering staff editor.\n");
                writeToRoster(STAFF);
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