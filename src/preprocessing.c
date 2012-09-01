#include <ctype.h>
#include <errno.h>

#include "preprocessing.h"
#include "codes.h"
#include "access.h"
#include "cutil.h"

static int visitor(qu_context_t *ctx,
    qu_ast_node *node,
    char *expr, qu_ast_node *expr_parent) {
    qu_nodedata *data = obstack_alloc(&ctx->parsing.pieces,
                                      sizeof(qu_nodedata));
    memset(data, 0, sizeof(qu_nodedata));
    if(!data)
        return -ENOMEM;
    data->expression = expr;
    data->expr_parent = expr_parent;
    node->userdata = data;
    if(node->tag) {
        data->kind = QU_MEMBER_SCALAR;
        if(node->kind == QU_NODE_MAPPING) {
           if(qu_map_get(node, "command-line")
                || qu_map_get(node, "command-line-incr")
                || qu_map_get(node, "command-line-decr")
                || qu_map_get(node, "command-line-enable")
                || qu_map_get(node, "command-line-disable")) {
                data->has_cli = 1;
                TAILQ_INSERT_TAIL(&ctx->cli_options, data, cli_lst);
            }
        }
    } else if(node->kind == QU_NODE_MAPPING) {
        data->kind = QU_MEMBER_STRUCT;
        qu_ast_node *key;
        CIRCLEQ_FOREACH(key, &node->children, lst) {
            char *mname = qu_node_content(key);
            if(!*mname || *mname == '_')
                continue;
            obstack_blank(&ctx->parsing.pieces, 0);
            obstack_grow(&ctx->parsing.pieces, expr, strlen(expr));
            obstack_grow(&ctx->parsing.pieces, ".", 1);
            qu_append_c_name(&ctx->parsing.pieces, mname);
            char *nexpr = (char *)obstack_finish(&ctx->parsing.pieces);
            visitor(ctx, key->value, nexpr, expr_parent);
        }
    }
    return 0;
}


int qu_config_preprocess(qu_context_t *ctx) {

    int rc = qu_parse_metadata(ctx->parsing.document, &ctx->meta);
    if(rc < 0)
        return rc;

    ctx->prefix = ctx->options.prefix;
    int len = strlen(ctx->prefix);
    ctx->macroprefix = obstack_copy0(&ctx->parsing.pieces, ctx->prefix, len);
    for(int i = 0; i < len; ++i)
        ctx->macroprefix[i] = toupper(ctx->macroprefix[i]);

    TAILQ_INIT(&ctx->cli_options);
    visitor(ctx, ctx->parsing.document, "", NULL);

    return 0;
}
