#include <assert.h>
#include <ctype.h>

#include "metadata.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"
#include "special/special.h"
#include "struct.h"
#include "context.h"

static void qu_visit_struct_children(struct qu_context *ctx,
    qu_ast_node *node, struct qu_config_struct *str)
{
    assert (node->kind == QU_NODE_MAPPING);
    qu_map_member *item;
    TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
        const char *mname = qu_node_content(item->key);
        if(!*mname || *mname == '_') {
            qu_special_parse(ctx, mname, item->value);
            continue;
        }
        if(!item->value->tag) {
            if(item->value->kind == QU_NODE_MAPPING) {
                struct qu_config_struct *child = qu_struct_new(ctx, str);
                qu_visit_struct_children(ctx, item->value, child);
            } else {
                LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                    item->value->start_token,
                    "Untagged straw scalar");
            }
        }
    }
}

static void qu_config_set_prefix(struct qu_context *ctx) {
    ctx->prefix = ctx->options.prefix;
    int len = strlen(ctx->prefix);
    char * macroprefix = obstack_copy0(&ctx->parser.pieces,
        ctx->prefix, len);
    for(int i = 0; i < len; ++i)
        macroprefix[i] = toupper(macroprefix[i]);
    ctx->macroprefix = macroprefix;
}

void qu_config_preprocess(struct qu_context *ctx) {
    qu_parse_metadata(ctx);
    qu_config_set_prefix(ctx);
    ctx->root = qu_struct_new_root(ctx);
    qu_visit_struct_children(ctx, ctx->parser.document, ctx->root);
}
