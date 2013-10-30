#include "fwdecl.h"
#include "print.h"
#include "../context.h"

struct qu_fwdecl_node {
    TAILQ_ENTRY(qu_fwdecl_node) lst;
    struct qu_fwdecl_node *left;
    struct qu_fwdecl_node *right;
    qu_fwdecl_printer printer;
    void *printer_data;
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
    TAILQ_INIT(&ctx->fwdecl_index.list);
}

static void qu_fwdecl_node_init(struct qu_fwdecl_node *node, const char *name,
    qu_fwdecl_printer printer, void *printer_data)
{
    node->left = NULL;
    node->right = NULL;
    node->printer = printer;
    node->printer_data = printer_data;
    strcpy(node->name, name);
}

int qu_fwdecl_add(struct qu_context *ctx, const char *name,
    qu_fwdecl_printer printer, void *data)
{
    int nlen = strlen(name);
    struct qu_fwdecl_node **node = qu_fwdecl_find(&ctx->fwdecl_index, name);
    if(*node)
        return 0;
    *node = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_fwdecl_node) + nlen+1);
    qu_fwdecl_node_init(*node, name, printer, data);
    TAILQ_INSERT_TAIL(&ctx->fwdecl_index.list, *node, lst);
    return 1;
}

void qu_fwdecl_print_all(struct qu_context *ctx) {
    struct qu_fwdecl_node *node;
    TAILQ_FOREACH(node, &ctx->fwdecl_index.list, lst) {
        qu_code_print(ctx,
            "${sname};\n",
            "sname", node->name,
            NULL);
    }
    TAILQ_FOREACH(node, &ctx->fwdecl_index.list, lst) {
        node->printer(ctx, node->printer_data);
    }
}
