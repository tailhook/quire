#include <ctype.h>

#include "name.h"
#include <stdio.h>

const char *reserved_words[] = {
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


const char *qu_c_name(struct obstack *ob, const char *name) {
    obstack_blank(ob, 0);
    qu_append_c_name(ob, name);
    return obstack_finish(ob);
}

void qu_append_c_name(struct obstack *ob, const char *name) {
    const char **word;
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
    for(word = reserved_words; *word; ++word) {
        if(!strncmp(namestart, *word, len) && len == (int)strlen(*word)) {
            obstack_1grow(ob, '_');
            break;  // can't match keyword twice
        }
    }
}

void qu_append_c_name_upper(struct obstack *ob, const char *name) {
    if(isdigit(*name))
        obstack_1grow(ob, '_');
    while(*name) {
        if(isalnum(*name)) {
            obstack_1grow(ob, toupper(*name));
        } else {
            obstack_1grow(ob, '_');
        }
        ++name;
    }
}

void qu_print_c_string(FILE *file, const char *str) {
    const char *c;
    putc('"', file);
    for(c = str; *c; ++c) {
        if(*c < 32) {
            switch(*c) {
            case '\r': fprintf(file, "\\r"); break;
            case '\n': fprintf(file, "\\n"); break;
            case '\t': fprintf(file, "\\t"); break;
            default: fprintf(file, "\\x%02x", *c); break;
            }
            continue;
        } else if(*c == '\\' || *c == '"') {
            putc('\\', file);
            putc(*c, file);
        } else {
            putc(*c, file);
        }
    }
    putc('"', file);
}
