#include <assert.h>

#include "metadata.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"
#include "special/special.h"
#include "struct.h"
#include "context.h"

void qu_visit_struct_children(struct qu_context *ctx,
    qu_ast_node *node, struct qu_config_struct *str)
{
    assert (node->kind == QU_NODE_MAPPING);
    qu_map_member *item;
    TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
        const char *mname = qu_node_content(item->key);
        if(!*mname || *mname == '_') {
            qu_special_parse(ctx, mname, item->value);
        }
    }
}

void qu_config_preprocess(struct qu_context *ctx) {
    qu_parse_metadata(ctx);
    ctx->root = qu_struct_new_root(ctx);
    qu_visit_struct_children(ctx, ctx->parser.document, ctx->root);
}
