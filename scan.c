#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "scan.h"

FILE *scan_fg = NULL;
char *scan_fname = NULL;

int look;
int LineCount = 0;

void openFile(char *file) {
    if(scan_fg == NULL) {
        if((scan_fg = fopen(file, "rb")) == NULL) {
            printf("Error: unable to open file %s\n", file);
            exit(1);
        }
    }
    else {
        printf("Error: file %s already open\n", file);
        exit(1);
    }

    scan_fname = malloc(strlen(file) + 1);
    strcpy(scan_fname, file);

    getChar();

    LineCount = 1;

    return;
}

void closeFile(char *file) {
    if((scan_fg == NULL || scan_fname == NULL) && strcmp(file, scan_fname) != 0) {
        printf("Error: file %s not open\n", file);
        exit(1);
    }
    fclose(scan_fg);
    free(scan_fname);
    scan_fg = NULL;
    scan_fname = NULL;

    return;
}

void getChar() {
    look = getc(scan_fg);
    if(look == '\n') {
        ++LineCount;
    }
    return;
}

void skipComment() {
    if(look == '#') {
        while(look != EOF && look != '\n') {
            getChar();
        }
    }
    return;
}

void skipEscape() {
    if(look == '\\') {
        getChar();
        while(look != EOF && look != '\n') {
            // We don't want trailing keywords, so check for them
            if(!isspace(look)) {
                // If it's a comment, skip to end of line
                if(look == '#') {
                    printf("Skipping comment\n");
                    skipComment();
                }
                // If it's not a comment, error
                else {
                    printf("Error: trailing characters after newline escape\n");
                    exit(1);
                }
            }
            // Just a space, get next char
            else {
                getChar();
            }
        }
        getChar();
    }
    return;
}

void skipSpace() {
    if(look == '#') {
        skipComment();
    }
    while(look == ' ' || look == '\t' || look == '\\' || look == '#') {
        //getChar();
        /* newline extender */
        if(look == '\\') {
            skipEscape();
        }
        /* Remove comments */
        else if(look == '#') {
            skipComment();
        }
        else {
            getChar();
        }
    }

    return;
}

void skipWhite() {
    while(isspace(look) || look == '\\' || look == '#') {
        skipSpace();
        if(isspace(look)) {
            getChar();
        }
    }
    return;
}

/* Get a string */
void getString(struct Token *tok) {
    int x;
    bool escaped = false;
    if(look != '"') {
        printf("Error: quote expected\n");
        exit(1);
    }

    x = 0;

    tok->str[x++] = look;
    getChar();

    // Continue until we either hit a newline or a non-escaped "
    while(look != EOF && look != '\n' && (escaped || look != '"')) {
        if(x == TOKENLEN - 1) {
            printf("Error: Input name too long\n");
            exit(1);
        }
        // If look = \\ and last character was not an escape, treat it like an escape
        if(look == '\\' && !escaped) {
            escaped = true;
        }
        else {
            escaped = false;
        }

        tok->str[x] = look;

        getChar();

        ++x;
    }

    if(look == '"') {
        tok->str[x++] = look;
        getChar();
    }
    else {
        printf("Error: String missing end quote\n");
        exit(1);
    }

    if(isalnum(look)) {
        printf("Error: unknown character following string%c\n", look);
        exit(1);
    }

    tok->str[x] = 0;

    return;
}

/* Get an identifier */
void getName(struct Token *tok) {
    int x;

    if(!isalpha(look)) {
        printf("Error: name expected\n");
        exit(1);
    }
    for(x = 0; isalnum(look) || look == '_'; ++x) {
        if(x == TOKENLEN - 1) {
            printf("Error: Input name too long\n");
            exit(1);
        }
        tok->str[x] = look;
        getChar();
    }

    tok->str[x] = 0;

    skipSpace();

    return;
}

/* Get a number */
void getNum(struct Token *tok) {
    int x;

    if(!isdigit(look)) {
        printf("Error: Integer expected\n");
    }

    for(x = 0; isdigit(look) || look == '_'; ++x) {
        if(x == TOKENLEN - 1) {
            printf("Error: Input number too long");
        }
        if(look != '_') {
            tok->str[x] = look;
        }
        else {
            --x;
        }
        getChar();
    }

    if(isalpha(look)) {
        printf("Error: Invalid Integer");
    }

    tok->str[x] = 0;

    skipSpace();

    return;
}

/* Recognize an operator */
bool isOp(char c) {
    return (c == '+') || (c == '-') || (c == '*') ||
           (c == '/') || (c == '<') || (c == '>') ||
           (c == '!') || (c == '=') || (c == '#') ||
           (c == '|') || (c == '&') || (c == '%');
}

/* Get an operator */
void getOp(struct Token *tok) {
    int x;

    if(!isOp(look)) {
        printf("Error: Operator expected");
    }

    for(x = 0; isOp(look); ++x) {
        if(x == TOKENLEN - 1) {
            printf("Error: Operator string too long");
        }
        tok->str[x] = look;
        getChar();
    }

    tok->str[x] = 0;

    skipSpace();

    return;
}

void scan(struct Token *token) {
    //int x;
    if(token == 0) {
        printf("Internal Error: token pointer invalid\n");
        exit(1);
    }

    //tokentype = INVALID;
    token->type = INVALID;

    skipSpace();

    if(look == EOF) {
        token->type = ENDOFFILE;
        token->str[0] = 0;
        return;
    }

    if(isalpha(look)) {
        getName(token);
        token->type = NAME;
    }
    else if(isdigit(look)) {
        getNum(token);
        token->type = NUM;
    }
    else if(isOp(look)) {
        getOp(token);
        token->type = OP;
    }
    else if(look == '{' || look == '}') {
        token->str[0] = look;
        token->str[1] = 0;
        getChar();
        token->type = BRACE;
    }
    else if(look == '(' || look == ')') {
        token->str[0] = look;
        token->str[1] = 0;
        getChar();
        token->type = PAREN;
    }
    else if(look == '"') {
        getString(token);
        token->type = STRING;
    }
    else if(look == '\n' || look == '\r') {
        token->str[0] = look;
        token->str[1] = 0;
        getChar();
        token->type = NEWLINE;
    }
    else if(look == ',') {
        token->str[0] = look;
        token->str[1] = 0;
        getChar();
        token->type = COMMA;
    }
    else {
        printf("Unknown term %c\n", look);
        exit(1);
    }

    token->lineNumber = LineCount;

    skipSpace();
}























