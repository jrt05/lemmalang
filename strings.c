#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "strings.h"

struct strings_list {
    struct strings_list *next;
    char *label;
    char *str;
};

struct strings_list *str_list = 0;
int strings_count = 0;

char* get_string(char *str) {
    char *ret = 0;
    struct strings_list *temp = str_list;
    while(temp != 0) {
        if(strcmp(temp->str, str) == 0) {
            break;
        }
        temp = temp->next;
    }
    if(temp != 0) {
        ret = temp->label;
    }
    return ret;
}

char* add_string(char *str) {
    char *ret;
    struct strings_list *temp = str_list;
    /* See if string exists */
    while(temp != 0) {
        /* Found the string */
        if(strcmp(temp->str, str) == 0) {
            break;
        }
        temp = temp->next;
    }

    /* We didn't find our string */
    if(temp == 0) {
        char l[20];
        temp = malloc(sizeof(struct strings_list));
        temp->next = str_list;
        /* Copy label */
        snprintf(l, 20, "__S%d", strings_count++);
        temp->label = malloc(strlen(l) + 1);
        strcpy(temp->label, l);
        /* Copy string */
        temp->str = malloc(strlen(str) + 1);
        strcpy(temp->str, str);

        str_list = temp;
        ret = temp->label;
    }
    else {
        ret = 0;
    }

    return ret;
}
