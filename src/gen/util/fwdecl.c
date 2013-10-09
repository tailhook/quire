#include "fwdecl.h"
#include "print.h"
#include "../context.h"

struct qu_fwdecl_node {
    struct qu_fwdecl_node *left;
    struct qu_fwdecl_node *right;
    char name[];
};


static struct qu_fwdecl_node **qu_fwdecl_find(struct qu_fwdecl_index *idx,
    const char *name)
{
    struct qu_fwdecl_node **node = &idx->root;
    while(*node) {
        int rc = strcmp((*node)->name, name);
        if(rc == 0)
            return node;
        if(rc < 0)
            node = &(*node)->left;
        if(rc > 0)
            node = &(*node)->right;
    }
    return node;
}

void qu_fwdecl_init(struct qu_context *ctx) {
    ctx->fwdecl_index.root = NULL;
}

static void qu_fwdecl_node_init(struct qu_fwdecl_node *node, const char *name) {
    node->left = NULL;
    node->right = NULL;
    strcpy(node->name, name);
}

int qu_fwdecl_add(struct qu_context *ctx, const char *name) {
    int nlen = strlen(name);
    struct qu_fwdecl_node **node = qu_fwdecl_find(&ctx->fwdecl_index, name);
    if(*node)
        return 0;
    *node = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_fwdecl_node) + nlen+1);
    qu_fwdecl_node_init(*node, name);
    return 1;
}

static void qu_fwdecl_print_visit(struct qu_context *ctx,
    struct qu_fwdecl_node *node)
{
    if(!node)
        return;
    qu_fwdecl_print_visit(ctx, node->left);
    qu_code_print(ctx,
        "struct ${sname};\n",
        "sname", node,
        NULL);
    qu_fwdecl_print_visit(ctx, node->right);
}

void qu_fwdecl_print_all(struct qu_context *ctx) {
    qu_fwdecl_print_visit(ctx, ctx->fwdecl_index.root);
}
