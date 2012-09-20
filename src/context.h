#ifndef _H_CONTEXT
#define _H_CONTEXT

#include <sys/queue.h>

#include "options.h"
#include "metadata.h"
#include "yparser.h"


typedef struct qu_nodedata {
    int kind;
    qu_ast_node *node;
    char *expression;
    qu_ast_node *expr_parent;

    TAILQ_ENTRY(qu_nodedata) cli_lst;
    char *cli_name;
} qu_nodedata;

typedef struct {
    qu_parse_context parsing;
    qu_options_t options;
    qu_metadata_t meta;
    char *prefix;
    char *macroprefix;

    TAILQ_HEAD(qu_cli_options, qu_nodedata) cli_options;

    int node_level;
    char node_vars[16];
} qu_context_t;

_Static_assert(sizeof(qu_context_t) < 4096,
    "Context size is greater than defined in quire.h");

#endif // _H_CONTEXT