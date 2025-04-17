#include <stdio.h>
#include <string.h>
#include "dao.h"

/* Write to file */

/* Function to display all */

/* Search Function */

/* Data entry */
void dataEntry(Role role) {
    FILE *filePointer;

    filePointer = fopen("schoolRoster.txt", "w");

    if (filePointer == NULL) {
        printf("File failed to open.");
    }

    struct Person entry;
    char buffer[100];

    printf("Please enter the first name: ");
    fgets(entry.name.first_name, sizeof(entry.name.first_name), stdin);

    printf("Please enter the middle name: ");
    fgets(entry.name.middle_name, sizeof(entry.name.middle_name), stdin);

    printf("Please enter the last name: ");
    fgets(entry.name.last_name, sizeof(entry.name.last_name), stdin);

    /* Need to process these entries */
    printf("Please enter their month of birth (1-12): ");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%d", &entry.date_of_birth.month);

    printf("Please enter their day of birth (1-31): ");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%d", &entry.date_of_birth.day);

    printf("Please enter their year of birth: ");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%d", &entry.date_of_birth.year);

    /* Should the age be calculated based on entry? */

    printf("Gender: ");
    fgets(entry.gender, sizeof(entry.gender), stdin);

    /* Phone number check? */
    printf("Phone number: ");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%d", &entry.phone_number);

    printf("Email: ");
    fgets(entry.email, sizeof(entry.email), stdin);

    printf("Street address: ");
    fgets(entry.address.street, sizeof(entry.address.street), stdin);

    printf("Apartment number (If applicable): ");
    fgets(entry.address.apartment, sizeof(entry.address.apartment), stdin);

    printf("City: ");
    fgets(entry.address.city, sizeof(entry.address.city), stdin);

    /* Check for this? */
    printf("State (two letter code): ");
    fgets(entry.address.state, sizeof(entry.address.state), stdin);


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

    char buffer[10];
    int selectedOption;
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%d", &selectedOption);

    char continueEdit = 'y';
    //while (continueEdit == 'y') {
        switch (selectedOption) {
            case 1:
                printf("Entering display mode.\n");
                break;
            case 2:
                printf("Entering search mode.\n");
                break;
            case 3:
                printf("Entering student editor.\n");
                dataEntry( STUDENT);
                break;
            case 4:
                printf("Entering teacher editor.\n");
                dataEntry(TEACHER);
                break;
            case 5:
                printf("Entering staff editor.\n");
                dataEntry(STAFF);
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
    //}
    
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

    char buffer[10];
    char options;
    printf("Would you like to see the options? (y/n) ");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%c", &options);

    if (options == 'y' || options == 'Y') {
        displayOptions();
    }

    fclose(filePointer);

    return 0;
}