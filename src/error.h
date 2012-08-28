#ifndef _H_ERROR
#define _H_ERROR

#include <stdio.h>

#include "yparser.h"

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
int qu_has_error(qu_parse_context *);
int qu_print_error(qu_parse_context *, FILE *err);

#endif //_H_ERROR
