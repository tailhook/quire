#ifndef _H_CONTEXT
#define _H_CONTEXT

#include <sys/queue.h>
#include <stdio.h>

#include "options.h"
#include "cli.h"
#include "metadata.h"
#include "util/fwdecl.h"
#include "special/types.h"
#include "../yaml/parser.h"

typedef struct qu_context {
    qu_parse_context parser;
    qu_options_t options;
    qu_metadata_t meta;
    const char *prefix;
    const char *macroprefix;

    struct qu_fwdecl_index fwdecl_index;
    struct qu_class_index class_index;
    struct qu_cli_options cli_options;

    struct qu_config_struct *root;

    int node_level;
    char node_vars[16];

    FILE *out;
} qu_context_t;

_Static_assert(sizeof(qu_context_t) < 4096,
    "Context size is greater than defined in quire.h");

void qu_context_init(qu_context_t *ctx, jmp_buf *jmp);

#endif // _H_CONTEXT
