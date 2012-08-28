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

    int node_level;
    char node_vars[16];
} qu_context_t;

_Static_assert(sizeof(qu_context_t) < 4096,
    "Context size is greater than defined in quire.h");

#endif // _H_CONTEXT
