#ifndef _H_CUTIL_
#define _H_CUTIL_

#include <obstack.h>
#include <stdio.h>

const char *qu_c_name(struct obstack *ob, const char *name);
void qu_append_c_name(struct obstack *ob, const char *name);
void qu_print_c_string(FILE *file, const char *str);

#endif  //_H_CUTIL_
