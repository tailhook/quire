#ifndef QUIRE_H_UTIL_WRAP
#define QUIRE_H_UTIL_WRAP

struct obstack;

#include <stdio.h>

const char *qu_line_grow(struct obstack *buf,
    const char *ptr, const char *endptr, int width);

const char *qu_line_print(FILE *out,
    const char *ptr, const char *endptr, int width);

#endif  /* QUIRE_H_UTIL_WRAP */
