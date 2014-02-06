#include <stdlib.h>

#include "template.h"
#include "eval.h"
#include "../error.h"
#include "../yaml/access.h"

static qu_ast_node *qu_node_deep_copy(struct qu_parser *ctx,
    qu_ast_node *src, struct qu_var_frame *vars) {
    qu_map_member *mitem;
    qu_seq_member *sitem;

    if(src->kind == QU_NODE_ALIAS) {
        /*  Aliases are copied resolved  */
        return qu_node_deep_copy(ctx, src->val.alias_target, vars);
    }
    qu_ast_node *self = obstack_alloc(&ctx->pieces, sizeof(qu_ast_node));
    memset(self, 0, sizeof(qu_ast_node));
    self->kind = src->kind;
    self->ctx = ctx;  /* TODO(tailhook) probably remove */
    self->tag_token = src->tag_token;
    self->tag = src->tag;
    self->start_token = src->start_token;
    self->end_token = src->end_token;
    self->content = src->content;
    self->content_len = src->content_len;
    switch(self->kind) {
    case QU_NODE_MAPPING:
        TAILQ_INIT(&self->val.map_index.items);
        self->val.map_index.tree = NULL;
        TAILQ_FOREACH(mitem, &src->val.map_index.items, lst) {
            const char *strkey = qu_parse_content(mitem->key, &ctx->pieces);
            if(!strkey) {
                qu_err_node_warn(ctx->err, mitem->key, "Non-scalar key");
                continue;
            }
            qu_ast_node *knode = qu_node_deep_copy(ctx, mitem->key, vars);
            qu_eval_str(ctx, vars, strkey, knode,
                    &knode->content, &knode->content_len);
            qu_mapping_add(ctx, self,
                knode, knode->content,
                qu_node_deep_copy(ctx, mitem->value, vars));
        }
        break;
    case QU_NODE_SEQUENCE:
        TAILQ_INIT(&self->val.seq_index.items);
        TAILQ_FOREACH(sitem, &src->val.seq_index.items, lst) {
            qu_sequence_add(ctx, self,
                qu_node_deep_copy(ctx, sitem->value, vars));
        }
        break;
    case QU_NODE_SCALAR:
        break;
    default:
        abort();
    }
    return self;
}

qu_ast_node *qu_raw_template(struct qu_parser *ctx,
    qu_ast_node *node, struct qu_var_frame *vars)
{
    const int off = strlen("!Template:");
    qu_ast_node *tpl = qu_find_anchor(ctx,
        node->tag + off, strlen(node->tag+off));
    if(!tpl) {
        qu_err_node_error(ctx->err, node,
            "Can't find anchor \"%s\"", node->tag + off);
        return node;
    }
    return qu_node_deep_copy(ctx, tpl, vars);
}

