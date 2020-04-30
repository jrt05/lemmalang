#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "lema.h"       // Function definitions
#include "scan.h"       // tokenizer functions
#include "proc.h"       // Process and compile function
#include "global.h"     // Global structs
#include "strings.h"    // Strings defintions

struct Var;
//struct Func;

struct VarLevels {
    struct Var *variables;
} VarLevels = {0};

/* List of all variables in this level */
struct Var {
    struct Var *next;
    char *name;
    char *value;
    enum vtype {QWORD} type;
};

/* List of all functions */
//struct Func {
//    struct Func *next;
//    char *name;
//};

/* List of all strings */
struct Str {
    struct Str *next;
    char *name;
    int  count;
};

struct Var *vars = NULL;
struct Func *funcs = NULL;
struct Str *strs = NULL;


int main(int argc, char *argv[]) {
    char *infile = argv[1];

    if(argc != 2) {
        printf("Error: invalid arguments\nProper usage:\n\t./lema infile.rr\n");
        exit(1);
    }

    pass1(infile);       // Find all functions and global variables

    pass2(infile);      // Actually parse all functions and instructions

    cleanup();          // Clean up any storage

    return 0;
}

/* Setup initial asm statements */
void pass1(char *infile) {
    struct Token token;

    openFile(infile);

    skipWhite();
    scan(&token);

    // Parse through program all the way
    while(token.type != ENDOFFILE) {
        // Search for function
        if(strcmp(token.str, "func") == 0) {
            scan(&token);
            init_func(&token);
        }
        // Not a function, so assume global variable
        else {
            init_vars(&token);
        }
        skipWhite();
        scan(&token);
    }

    // Print variables in the proper location
    if(vars != 0 || strs != 0) {
        struct Var *temp = vars;
        struct Str *stemp = strs;

        printf("section .data\n");

        // Put strings in initialized storage
        while(stemp != 0) {
            // We have a string
            char *label = add_string(stemp->name);
            if(label) {
                printf("\t%s\tdb\t%s,0\n", label, stemp->name);
            }
            stemp = stemp->next;
        }

        // Put variable in initialized storage
        while(temp != 0) {
            // We have a value for this variable
            if(temp->value != 0) {
                printf("\t%s", temp->name);
                switch(temp->type) {
                    case QWORD:
                    printf("\tdq");
                    break;
                    default:
                    printf("\tdb");
                    break;
                }
                printf("\t%s\n", temp->value);
            }
            temp = temp->next;
        }

        printf("section .bss\n");

        temp = vars;
        // Put variable in uninitialized storage
        while(temp != 0) {
            // We have no value for this variable
            if(temp->value == 0) {
                printf("\t%s", temp->name);
                switch(temp->type) {
                    case QWORD:
                    printf("\tresq\t1\n");
                    break;
                    default:
                    printf("\tresb");
                    break;
                }
            }
            temp = temp->next;
        }
    }
    else {
        printf("section .data\n");
        printf("section .bss\n");
    }

    closeFile(infile);

    return;
}

/* Parse through the program */
void pass2(char *infile) {
    struct Token token;

    openFile(infile);

    skipWhite();
    scan(&token);

    // Parse through program all the way
    while(token.type != ENDOFFILE) {
        // Search for function
        if(strcmp(token.str, "func") == 0) {
            scan(&token);
            procFunc(&token);
        }
        // Not a function, so assume global variable
        else {
            // Skip variable, so scan until newline
            while(token.type != NEWLINE) {
                scan(&token);
            }
        }
        skipWhite();
        scan(&token);
    }

    closeFile(infile);

    return;
}

void init_vars(struct Token *token) {
    struct Var temp = {0};
    struct Var *vptr;
    char name[TOKENLEN];

    if(token->type == NAME && strcmp(token->str, "qword") == 0) {
        temp.type = QWORD;
        temp.value = 0;
    }
    else {
        printf("Error: Invalid global keyword %s\n", token->str);
        exit(1);
    }

    scan(token); // Get variable name

    if(token->type == NAME) {
        strcpy(name, token->str);
    }
    else {
        printf("Error: Invalid variable name %s\n", token->str);
        exit(1);
    }

    vptr = vars;

    // Check if variable already exists
    while(vptr != NULL) {
        if(strcmp(vptr->name, token->str) == 0) {
            printf("Error: Duplicate variable name %s\n", name);
            exit(1);
        }
        vptr = vptr->next;
    }

    scan(token);     // Get '=' sign if exists

    // Get value for variable if one exists
    if(token->type == OP && strcmp(token->str, "=") == 0) {
        scan(token);     // Get value
        if(temp.type == QWORD) {
            if(token->type == NUM) {
                temp.value = malloc(strlen(token->str) + 1);
                strcpy(temp.value, token->str);
            }
            else {
                printf("Error: Invalid %s value for qword\n", token->str);
                exit(1);
            }
        }
    }
    else if(token->type != NEWLINE) {
        printf("Error: malformed variable %s\n", name);
        exit(1);
    }

    // New variable is good, create new element in chain
    vptr = malloc(sizeof(struct Var));
    vptr->name = malloc(strlen(name) + 1);
    strcpy(vptr->name, name);
    vptr->type = temp.type;
    vptr->value = temp.value;
    vptr->next = vars;
    vars = vptr;

    return;
}

void init_func(struct Token *token) {
    char name[TOKENLEN];
    int inBlock = 0;
    struct Func *temp;
    int stringCount = 0;

    strcpy(name, token->str);

    if(token->type == NAME) {
        scan(token);
    }
    else {
        printf("Error: Invalid function name %s\n", token->str);
        exit(1);
    }

    temp = funcs;
    // Find duplicate function names
    while(temp != NULL) {
        if(strcmp(name, temp->name) == 0) {
            printf("Error: Duplicate function name %s\n", name);
            exit(1);
        }
        temp = temp->next;
    }

    temp = malloc(sizeof(struct Func));
    temp->next = funcs;
    temp->name = malloc(strlen(name) + 1);
    strcpy(temp->name, name);

    funcs = temp;


    // Here is where we want to get args and return type

    while(token->type != ENDOFFILE && token->type != BRACE) {
        scan(token);
    }

    // We're searching for literals here (string and array)
    if(token->type == BRACE && strcmp(token->str, "{") == 0) {
        ++inBlock;
        // Keep going through block until we hit end function brace
        while(token->type != ENDOFFILE && inBlock != 0) {
            scan(token);
            if(token->type == BRACE && strcmp(token->str, "{") == 0) {
                ++inBlock;
            }
            else if(token->type == BRACE && strcmp(token->str, "}") == 0) {
                --inBlock;
            }
            /* Create a string variable */
            else if(token->type == STRING) {
                struct Str *str_temp;
                str_temp = malloc(sizeof(struct Str));
                str_temp->name = malloc(strlen(token->str) + 1);
                str_temp->count = stringCount++;
                strcpy(str_temp->name, token->str);
                str_temp->next = strs;
                strs = str_temp;
            }
        }
        if(inBlock != 0) {
            printf("Error: Function %s invalid, missing end brace\n", name);
            exit(1);
        }
    }
    else {
        printf("Error: Function %s invalid, missing brace\n", name);
        exit(1);
    }

    return;
}

void cleanup() {

    while(funcs != NULL) {
        struct Func *temp;
        free(funcs->name);
        temp = funcs->next;
        free(funcs);
        funcs = temp;
    }
    while(vars != NULL) {
        struct Var *temp;
        free(vars->name);
        if(vars->value != 0) {
            free(vars->value);
        }
        temp = vars->next;
        free(vars);
        vars = temp;
    }
    while(strs != NULL) {
        struct Str *temp;
        free(strs->name);
        temp = strs->next;
        free(strs);
        strs = temp;
    }
    return;
}






















