#ifndef _H_CONTEXT
#define _H_CONTEXT

#include "options.h"
#include "metadata.h"
#include "yparser.h"

typedef struct {
    qu_parse_context parsing;
    qu_options_t options;
    qu_metadata_t meta;
    char *prefix;
    char *macroprefix;
} qu_context_t;

#endif // _H_CONTEXT
