#ifndef QUIRE_H_UTIL_PARSE
#define QUIRE_H_UTIL_PARSE

/*  Public API  */
int qu_parse_int(const char *value, long *result);
int qu_parse_float(const char *value, double *result);
int qu_parse_bool(const char *value, int *result);

#endif  /* QUIRE_H_UTIL_PARSE */
