#ifndef SCAN_H
#define SCAN_H
#include <stdbool.h>

struct Token {
    #define TOKENLEN 100
    char str[TOKENLEN];
    #define NAME      0x0001  // Token is a name
    #define NUM       0x0002  // Token is a number
    #define OP        0x0003  // Token is a operator
    #define PAREN     0x0004  // Token is a parenthesis
    #define BRACE     0x0005  // Token is a brace
    #define KEY       0x0006  // Token is a keyword
    #define QUOTE     0x0007  // Token is a quote
    #define NEWLINE   0x0008  // Token is a newline
    #define COMMA     0x0009  // Token is a comma
    #define STRING    0x000a  // Token is a string
    #define ENDOFFILE 0x000b  // EOF has been reached
    #define INVALID   0x0000
    int type;
    int lineNumber;
};

void getChar();
void openFile(char *fileName);
void closeFile(char *fileName);
void skipComment();
void skipEscape();
void skipSpace();
void skipWhite();
void getString(struct Token *);
void getName(struct Token *);
void getNum(struct Token *);
bool isOp(char c);
void getOp(struct Token *);
void scan(struct Token *);

#endif /* SCAN_H */





















