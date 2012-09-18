#ifndef _H_EVAL
#define _H_EVAL

#include "yparser.h"

void qu_eval_int(qu_parse_context *info,
    char *value, size_t vlen, long *result);
void qu_eval_float(qu_parse_context *info,
    char *value, size_t vlen, double *result);
void qu_eval_str(qu_parse_context *info,
    char *value, size_t vlen, char **result, int *rlen);

#endif // _H_EVAL
