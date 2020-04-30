#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "proc.h"
#include "scan.h"
#include "strings.h"

struct LocalVars {
    struct LocalVars *next; // Next in chain
    char *type;             // Datatype
    char *name;             // Variable name
    int offset;             // Offset in stack if not static
    bool isStatic;          // If it's static or not
};

struct VarList {
    struct LocalVars *var;
};

void scanVars(struct VarList *vars, struct Token *token);
void block(char *breakLabel, struct Token *token);
void assignment(char *breakLabel, struct Token *token);
void matchString(struct Token *, char *);
void boolCompare(struct Token *);
void boolExpression(struct Token *);
void expression(struct Token *);
void term(struct Token *);
void factor(struct Token *);

void boolOr(struct Token *);
void boolAnd(struct Token *);

void boolEquals(struct Token *);
void boolNotEquals(struct Token *);
void boolGreater(struct Token *);
void boolLess(struct Token *);
void boolGEquals(struct Token *);
void boolLEquals(struct Token *);

void add(struct Token *);
void sub(struct Token *);

void multiply(struct Token *);
void divide(struct Token *);
void modulo(struct Token *);

void push(char *);
void pop(char *);
void createLabel(char *);

/* Found a function, process it */
void procFunc(struct Token *token) {
    char funcName[TOKENLEN];
    struct VarList local = {0};

    strcpy(funcName, token->str);

    while(token->type != BRACE) {
        scan(token);
    }

    // Find opening brace
    if(strcmp(token->str, "{") != 0) {
        printf("Error: missing { on Line = %d\n", token->lineNumber);
        exit(1);
    }

    scan(token);

    // Verify newline
    if(strcmp(token->str, "\n") != 0) {
        printf("Error: Newline expected after brace on line = %d\n", token->lineNumber);
        exit(1);
    }

    skipWhite();
    scan(token);

    printf("section .data\n");

    scanVars(&local, token);

    // Scan for any static variables
    printf("section .bss\n");
    printf("section .text\n");
    printf("global %s\n", funcName);
    printf("%s:\n", funcName);

    block("", token);

    if(strcmp(token->str, "}") != 0) {
        printf("Error: Missing end brace for func %s\n", funcName);
        exit(1);
    }

    printf("\tret\n");

    scan(token);
    skipWhite();

    return;
}

/* Process a block */
void block(char *breakLabel, struct Token *token) {
    bool endloop = false;
    while(token->type != ENDOFFILE && !endloop) {
        switch(token->type) {
            case NAME:
            assignment(breakLabel, token);
            break;
            case BRACE:
            if(strcmp(token->str, "}") == 0) {
                endloop = true;
            }
            break;
            default:
            printf("Error: invalid keyword %s. Line = %d\n", token->str, token->lineNumber);
            exit(1);
            break;
        }
    }

    return;
}

void scanVars(struct VarList *vars, struct Token *token) {
    return;
}

void assignment(char *breakLabel, struct Token *token) {
    char name[TOKENLEN];
    strcpy(name, token->str);

    scan(token);

    // Found an assignment statement
    if(strcmp(token->str, "=") == 0) {
        // Verify variable exists
        scan(token);
        boolExpression(token);
        if(token->type != NEWLINE) {
            printf("Error: invalid keyword %s on line=%d\n", token->str, token->lineNumber);
            exit(1);
        }
        printf("\tmov\t[%s],rax\n", name);
        //scan(token);
    }
    else if(strcmp(token->str, "(") == 0) {
        // Verify function exists
        scan(token);
        while(strcmp(token->str, ")") != 0) {
            boolExpression(token);
            if(token->type == COMMA) {
                scan(token);
            }
            printf("\tpush\trax\n");
        }

        scan(token);

        if(token->type != NEWLINE) {
            printf("Error: invalid keyword %s on line=%d\n", token->str, token->lineNumber);
            exit(1);
        }

        printf("\tcall\t%s\n", name);
    }

    // Verify nothing follows our assignment
    if(token->type != NEWLINE) {
        printf("Error: invalid keyword %s on line=%d\n", token->str, token->lineNumber);
        exit(1);
    }

    // Get next token
    skipWhite();
    scan(token);

    return;
}

void ident(struct Token *token) {
    char name[TOKENLEN];
    strcpy(name, token->str);

    scan(token);

    // Found a function
    if(strcmp(token->str, "(") == 0) {
        // Verify function exists
        scan(token);
        while(strcmp(token->str, ")") != 0) {
            boolExpression(token);
            if(token->type == COMMA) {
                scan(token);
            }
            printf("\tpush\trax\n");
        }

        scan(token);

        printf("\tcall\t%s\n", name);
    }
    else {
        printf("\tmov\trax,[%s]\n", name);
    }

    return;
}

void factor(struct Token *token) {
    bool notflag = false;
    bool signedFlag = false;
    if(token->type == OP) {
        if(strcmp(token->str, "!") == 0) {
            notflag = true;
            scan(token);
        }
    }

    if(token->type == OP) {
        if(strcmp(token->str, "-") == 0) {
            signedFlag = true;
            scan(token);
        }
    }

    if(token->type == PAREN) {
        matchString(token, "(");
        scan(token);
        boolExpression(token);
        matchString(token, ")");
        scan(token);
        if(signedFlag) {
            printf("\tneg\trax\n");
        }
    }
    else if(token->type == NAME) {
        ident(token);
        if(signedFlag) {
            printf("\tneg\trax\n");
        }
    }
    else if(token->type == NUM) {
        char sign[2] = "";
        if(signedFlag) {
            sign[0] = '-';
        }
        printf("\tmov\trax,%s%s\n", sign, token->str);
        scan(token);
    }

    // We had a not flag, so negate the value
    if(notflag) {
        char label1[20];
        char label2[20];
        createLabel(label1);
        createLabel(label2);
        printf("\ttest\trax,rax\n");
        printf("\tjz\t%s\n", label1);
        printf("\txor\trax,rax\n");
        printf("\tjmp\t%s\n", label2);
        printf("%s:\n", label1);
        printf("\tmov\trax,1\n");
        printf("%s:\n", label2);
    }
    return;
}

void term(struct Token *token) {
    factor(token);

    while(token->type == OP) {
        if(strcmp(token->str, "*") == 0) {
            multiply(token);
        }
        else if(strcmp(token->str, "/") == 0) {
            divide(token);
        }
        else if(strcmp(token->str, "%") == 0) {
            modulo(token);
        }
        else {
            break;
        }
    }
    return;
}

void expression(struct Token *token) {
    term(token);

    while(token->type == OP) {
        if(strcmp(token->str, "+") == 0) {
            add(token);
        }
        else if(strcmp(token->str, "-") == 0) {
            sub(token);
        }
        else {
            break;
        }
    }

    return;
}

void boolCompare(struct Token *token) {
    expression(token);
    while(token->type == OP) {
        if(strcmp(token->str, "==") == 0) {
            boolEquals(token);
        }
        else if(strcmp(token->str, "!=") == 0) {
            boolNotEquals(token);
        }
        else if(strcmp(token->str, ">=") == 0) {
            boolGEquals(token);
        }
        else if(strcmp(token->str, "<=") == 0) {
            boolLEquals(token);
        }
        else if(strcmp(token->str, ">") == 0) {
            boolGreater(token);
        }
        else if(strcmp(token->str, "<") == 0) {
            boolLess(token);
        }
        else {
            break;
        }
    }
}

void boolExpression(struct Token *token) {
    boolCompare(token);

    while(token->type == OP) {
        if(strcmp(token->str, "||") == 0) {
            boolOr(token);
        }
        else if(strcmp(token->str, "&&") == 0) {
            boolAnd(token);
        }
        else {
            break;
        }
    }

    return;
}

void multiply(struct Token *token) {
    scan(token);
    push("rax");
    factor(token);
    pop("rbx");
    printf("\tmul\trbx\n");
    printf("\n");
    return;
}

void divide(struct Token *token) {
    scan(token);
    push("rax");
    factor(token);
    printf("\tmov\trbx,rax\n");
    pop("rax");
    printf("\txor\trdx,rdx\n");
    printf("\tdiv\trbx\n");
    printf("\n");
    return;
}

void modulo(struct Token *token) {
    scan(token);
    push("rax");
    factor(token);
    printf("\tmov\trbx,rax\n");
    pop("rax");
    printf("\txor\trdx,rdx\n");
    printf("\tdiv\trbx\n");
    printf("\tmov\trax,rdx\n");
    printf("\n");
    return;
}

void add(struct Token *token) {
    scan(token);
    push("rax");
    term(token);
    pop("rbx");
    printf("\tadd\trax,rbx\n");
    printf("\n");
    return;
}

void sub(struct Token *token) {
    scan(token);
    push("rax");
    term(token);
    printf("\tmov\trbx,rax\n");
    pop("rax");
    printf("\tsub\trax,rbx\n");
    printf("\n");
    return;
}

void boolOr(struct Token *token){
    scan(token);
    push("rax");
    boolCompare(token);
    pop("rbx");
    printf("\tbool or\n");
    return;
}

void boolAnd(struct Token *token){
    scan(token);
    return;
}

void boolEquals(struct Token *token){
    char label[20];
    createLabel(label);
    scan(token);
    push("rax");
    expression(token);
    pop("rbx");
    printf("\tcmp\trax,rbx\n");
    printf("\tmov\trax,1\n");
    printf("\tje\t%s\n", label);
    printf("\txor\trax,rax\n");
    printf("%s:\n", label);
    printf("\n");
    return;
}

void boolNotEquals(struct Token *token){
    char label[20];
    createLabel(label);
    scan(token);
    push("rax");
    expression(token);
    pop("rbx");
    printf("\tcmp\trax,rbx\n");
    printf("\tmov\trax,1\n");
    printf("\tjne\t%s\n", label);
    printf("\txor\trax,rax\n");
    printf("%s:\n", label);
    printf("\n");
    return;
}

void boolGEquals(struct Token *token){
    char label[20];
    createLabel(label);
    scan(token);
    push("rax");
    expression(token);
    pop("rbx");
    printf("\tcmp\trax,rbx\n");
    printf("\tmov\trax,1\n");
    printf("\tjge\t%s\n", label);
    printf("\txor\trax,rax\n");
    printf("%s:\n", label);
    printf("\n");
    return;
}

void boolLEquals(struct Token *token){
    char label[20];
    createLabel(label);
    scan(token);
    push("rax");
    expression(token);
    pop("rbx");
    printf("\tcmp\trax,rbx\n");
    printf("\tmov\trax,1\n");
    printf("\tjle\t%s\n", label);
    printf("\txor\trax,rax\n");
    printf("%s:\n", label);
    printf("\n");
    return;
}

void boolLess(struct Token *token){
    char label[20];
    createLabel(label);
    scan(token);
    push("rax");
    expression(token);
    pop("rbx");
    printf("\tcmp\trax,rbx\n");
    printf("\tmov\trax,1\n");
    printf("\tjl\t%s\n", label);
    printf("\txor\trax,rax\n");
    printf("%s:\n", label);
    printf("\n");
    return;
}

void boolGreater(struct Token *token){
    char label[20];
    createLabel(label);
    scan(token);
    push("rax");
    expression(token);
    pop("rbx");
    printf("\tcmp\trax,rbx\n");
    printf("\tmov\trax,1\n");
    printf("\tjg\t%s\n", label);
    printf("\txor\trax,rax\n");
    printf("%s:\n", label);
    printf("\n");
    return;
}

void matchString(struct Token *token, char *want) {
    if(strcmp(token->str, want) != 0) {
        printf("Error: expected %s on line=%d\n", want, token->lineNumber);
        exit(1);
    }
    return;
}

void createLabel(char *buf) {
    static unsigned int count = 0;
    snprintf(buf, 20, ".L%u", count++);
    return;
}

void push(char *str) {
    printf("\tpush\t%s\n", str);
    return;
}

void pop(char *str) {
    printf("\tpop\t%s\n", str);
    return;
}

























