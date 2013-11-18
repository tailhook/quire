#include "template.h"
#include "../error.h"

qu_ast_node *qu_raw_template(struct qu_parser *ctx, qu_ast_node *node)
{
    const int off = strlen("!Template:");
    qu_ast_node *tpl = qu_find_anchor(ctx,
        node->tag + off, strlen(node->tag+off));
    if(!tpl) {
        qu_err_node_error(ctx->err, node,
            "Can't find anchor \"%s\"", node->tag + off);
        return node;
    }
    return qu_node_deep_copy(ctx, tpl);
}

