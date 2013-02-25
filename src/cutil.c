#include <ctype.h>

#include "cutil.h"
#include <stdio.h>

char *reserved_words[] = {
    "auto",
    "break",
    "case",
    "char",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "entry",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "int",
    "long",
    "register",
    "return",
    "short",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "while",
    "enum",
    "void",
    "const",
    "signed",
    "volatile",
    NULL
    };


char *qu_c_name(struct obstack *ob, char *name) {
    obstack_blank(ob, 0);
    qu_append_c_name(ob, name);
    return obstack_finish(ob);
}

void qu_append_c_name(struct obstack *ob, char *name) {
    char *namestart = obstack_base(ob) + obstack_object_size(ob);
    if(isdigit(*name))
        obstack_1grow(ob, '_');
    while(*name) {
        if(isalnum(*name)) {
            obstack_1grow(ob, *name);
        } else {
            obstack_1grow(ob, '_');
        }
        ++name;
    }
    int len = (char *)obstack_base(ob) + obstack_object_size(ob) - namestart;
    for(char **word = reserved_words; *word; ++word) {
        if(!strncmp(namestart, *word, len)) {
            obstack_1grow(ob, '_');
            break;  // can't match keyword twice
        }
    }
    obstack_1grow(ob, 0);
}
