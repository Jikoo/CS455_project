
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

typedef enum {
    SCHOOL_COUNSELOR,
    DEAN,
    PRINCIPAL,
    ASSISTANT_PRINCIPAL,
    JANITOR,
    LIBRARIAN,
    ATHLETICS_DIRECTOR
} Position;

typedef struct {
    char street[50];
    char apartment[10];
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
    #include "person.h"
};

struct Teacher {
    #include "person.h"
    int salary;
    Department department;
};

struct Student {
    #include "person.h"
    char grade;
};

struct Staff {
    #include "person.h"
    int salary;
    Position position;
};
