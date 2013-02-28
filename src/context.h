#ifndef _H_CONTEXT
#define _H_CONTEXT

#include <sys/queue.h>

#include "options.h"
#include "metadata.h"
#include "yparser.h"

typedef struct qu_nodedata {
    int kind;
    int type;
    qu_ast_node *node;
    char *expression;
    qu_ast_node *expr_parent;

    union {
        struct {
            char *typename;
        } custom;
		struct {
			TAILQ_ENTRY(qu_nodedata) lst;
			char *membername;
		} array;
		struct {
			TAILQ_ENTRY(qu_nodedata) lst;
			char *keyname;
			char *valuename;
		} mapping;
    } data;

    TAILQ_ENTRY(qu_nodedata) cli_lst;
    char *cli_name;
} qu_nodedata;


typedef struct qu_context_s {
    qu_parse_context parsing;
    qu_options_t options;
    qu_metadata_t meta;
    char *prefix;
    char *macroprefix;

    TAILQ_HEAD(qu_cli_options, qu_nodedata) cli_options;

    int node_level;
    char node_vars[16];

	TAILQ_HEAD(qu_array_list, qu_nodedata) arrays;
	TAILQ_HEAD(qu_mapping_list, qu_nodedata) mappings;
} qu_context_t;

_Static_assert(sizeof(qu_context_t) < 4096,
    "Context size is greater than defined in quire.h");

#endif // _H_CONTEXT
