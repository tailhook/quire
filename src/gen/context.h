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

typedef struct qu_nodedata {
    int kind;
    int type;
    qu_ast_node *node;
    const char *expression;
    qu_ast_node *expr_parent;

    union {
        struct {
            const char *typename;
        } custom;
		struct {
			TAILQ_ENTRY(qu_nodedata) lst;
			const char *membername;
		} array;
		struct {
			TAILQ_ENTRY(qu_nodedata) lst;
			const char *keyname;
			const char *valuename;
		} mapping;
    } data;

    TAILQ_ENTRY(qu_nodedata) cli_lst;
    const char *cli_name;
} qu_nodedata;


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

	TAILQ_HEAD(qu_array_list, qu_nodedata) arrays;
	TAILQ_HEAD(qu_mapping_list, qu_nodedata) mappings;
} qu_context_t;

_Static_assert(sizeof(qu_context_t) < 4096,
    "Context size is greater than defined in quire.h");

void qu_context_init(qu_context_t *ctx, jmp_buf *jmp);

#endif // _H_CONTEXT
