#include "anchors.h"
#include "parser.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

struct qu_anchor_node {
    const char *name;
    int name_len;
    qu_ast_node *data;
    struct qu_anchor_node *left;
    struct qu_anchor_node *right;
};

struct qu_anchor_node **qu_anchor_find(struct qu_anchor_index *idx,
    const char *name, int name_len) {
    struct qu_anchor_node **node = &idx->node;
    while(*node) {
        int cmp = memcmp((*node)->name, name, min((*node)->name_len, name_len));
        if(!cmp && (*node)->name_len != name_len)
            cmp = (*node)->name_len > name_len ? 1 : -1;
        if(!cmp)
            return node;
        if(cmp < 0)
            node = &(*node)->left;
        if(cmp > 0)
            node = &(*node)->right;
    }
    return node;
}

void qu_anchor_init(struct qu_anchor_node *anch, qu_ast_node *node) {
    anch->name = (char *)node->anchor->data+1;
    anch->name_len = node->anchor->bytelen-1;
    anch->data = node;
    anch->left = NULL;
    anch->right = NULL;
}

void qu_insert_anchor(qu_parse_context *ctx, qu_ast_node *node) {
    struct qu_anchor_node **a;

    a = qu_anchor_find(&ctx->anchor_index,
        (char *)node->anchor->data+1, node->anchor->bytelen-1);
    if(*a) {
        (*a)->data = node;
    } else {
        *a = obstack_alloc(&ctx->pieces, sizeof(struct qu_anchor_node));
        qu_anchor_init(*a, node);
    }
}
qu_ast_node *qu_find_anchor(qu_parse_context *ctx,
    const char *name, int name_len)
{
    struct qu_anchor_node **a;

    a = qu_anchor_find(&ctx->anchor_index, name, name_len);
    if(!*a)
        return NULL;
    return (*a)->data;
}

