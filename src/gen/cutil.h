#ifndef _H_CUTIL_
#define _H_CUTIL_

#include <obstack.h>
#include <stdio.h>

char *qu_c_name(struct obstack *ob, char *name);
void qu_append_c_name(struct obstack *ob, char *name);
void qu_print_c_string(FILE *file, char *str);

#endif  //_H_CUTIL_
