#include <stdlib.h>

#include "node.h"
#include "parser.h"
#include "codes.h"
#include "anchors.h"
#include "access.h"
#include "assert.h"


static qu_ast_node *qu_new_node(struct qu_parser *ctx) {
    qu_ast_node *node = obstack_alloc(&ctx->pieces, sizeof(qu_ast_node));
    memset(node, 0, sizeof(qu_ast_node));
    node->kind = QU_NODE_UNKNOWN;
    node->anchor = ctx->cur_anchor;
    node->ctx = ctx;  /* TODO(tailhook) probably remove */
    if(ctx->cur_anchor) {
        qu_insert_anchor(ctx, node);
        ctx->cur_anchor = NULL;
    }
    node->tag_token = ctx->cur_tag;
    if(ctx->cur_tag) {
        ctx->cur_tag = NULL;
        char *tag = obstack_alloc(&ctx->pieces, node->tag_token->bytelen+1);
        memcpy(tag, node->tag_token->data, node->tag_token->bytelen);
        tag[node->tag_token->bytelen] = 0;
        node->tag = tag;
    }
    return node;
}

qu_ast_node *qu_new_text_node(struct qu_parser *ctx, qu_token *tok) {
    qu_ast_node *node = qu_new_node(ctx);
    node->kind = QU_NODE_SCALAR;
    node->start_token = tok;
    node->end_token = tok;
    // TODO(tailhook) parse content
    return node;
}
qu_ast_node *qu_new_raw_text_node(struct qu_parser *ctx,
    const char *data) {
    qu_ast_node *node = qu_new_node(ctx);
    node->kind = QU_NODE_SCALAR;
    node->start_token = NULL;
    node->end_token = NULL;
    node->content = data;
    node->content_len = strlen(data);
    return node;
}

qu_ast_node *qu_new_alias_node(struct qu_parser *ctx,
    qu_token *tok, qu_ast_node *target) {
    qu_ast_node *node = qu_new_node(ctx);
    node->start_token = tok;
    node->end_token = tok;
    node->kind = QU_NODE_ALIAS;
    node->val.alias_target = target;
    return node;
}

qu_ast_node *qu_new_mapping_node(struct qu_parser *ctx,
    qu_token *start_tok) {
    qu_ast_node *node = qu_new_node(ctx);
    node->start_token = start_tok;
    node->kind = QU_NODE_MAPPING;
    TAILQ_INIT(&node->val.map_index.items);
    node->val.map_index.tree = NULL;
    return node;
}

qu_ast_node *qu_new_sequence_node(struct qu_parser *ctx,
    qu_token *start_tok) {
    qu_ast_node *node = qu_new_node(ctx);
    node->kind = QU_NODE_SEQUENCE;
    node->start_token = start_tok;
    TAILQ_INIT(&node->val.seq_index.items);
    return node;
}
void qu_sequence_add(struct qu_parser *ctx,
    qu_ast_node *seq, qu_ast_node *child) {
    qu_seq_member *mem = obstack_alloc(&ctx->pieces,
                                       sizeof(qu_map_member));
    mem->value = child;
    TAILQ_INSERT_TAIL(&seq->val.seq_index.items, mem, lst);
}
int qu_mapping_add(struct qu_parser *ctx,
    qu_ast_node *map, qu_ast_node *knode, const char *key, qu_ast_node *value)
{
    assert(value);
    qu_map_member **targ;
    targ = qu_find_node(&map->val.map_index.tree, key);
    if(*targ) {
        return 0;
    } else {
        *targ = obstack_alloc(&ctx->pieces,
                              sizeof(qu_map_member));
        (*targ)->key = knode;
        (*targ)->left = NULL;
        (*targ)->right = NULL;
        TAILQ_INSERT_TAIL(&map->val.map_index.items, *targ, lst);
        (*targ)->value = value;
        return 1;
    }
}

