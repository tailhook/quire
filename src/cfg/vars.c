#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vars.h"
#include "../yaml/access.h"
#include "../yaml/anchors.h"
#include "context.h"

#define min(a, b) ((a) > (b) ? (b) : (a))

struct qu_variable {
    struct qu_variable *left;
    struct qu_variable *right;
    char *name;
    int name_len;
    char *value;
    int value_len;
};

static struct qu_variable **qu_var_find(struct qu_vars_index *idx,
    const char *name, int nlen)
{
    struct qu_variable **node = &idx->root;
    while(*node) {
        int cmp = memcmp((*node)->name, name, min((*node)->name_len, nlen));
        if(!cmp && (*node)->name_len != nlen)
            cmp = (*node)->name_len > nlen ? 1 : -1;
        if(!cmp)
            return node;
        if(cmp < 0)
            node = &(*node)->left;
        if(cmp > 0)
            node = &(*node)->right;
    }
    return node;
}

static struct qu_variable *qu_var_new(qu_config_context *ctx, const char *name)
{
    struct qu_variable *var = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_variable) + strlen(name) + 1);
    var->left = NULL;
    var->right = NULL;
    var->name = ((char *)var) + sizeof(struct qu_variable);
    var->name_len = strlen(name);
    memcpy(var->name, name, var->name_len);
    var->name[var->name_len] = 0;
    return var;
}

int qu_set_string(qu_config_context *ctx, const char *name, const char *data) {
	int dlen = strlen(data);
    struct qu_variable **var = qu_var_find(&ctx->variables, name, strlen(name));
    if(!*var)
        *var = qu_var_new(ctx, name);
    (*var)->value = obstack_copy0(&ctx->parser.pieces, data, dlen);
    (*var)->value_len = dlen;
    return 0;
}

int qu_string_var(qu_config_context *ctx, const char *name, int nlen,
    const char **data, int *dlen)
{
    struct qu_variable **ptr = qu_var_find(&ctx->variables, name, nlen);
    if(*ptr) {
        *data = (*ptr)->value;
        *dlen = (*ptr)->value_len;
        return 1;
    }
    return 0;
}

int qu_anchor_var(qu_config_context *ctx, const char *name, int nlen,
    const char **data, int *dlen)
{
    qu_ast_node *node = qu_find_anchor(&ctx->parser, name, nlen);
    if(node) {
        *data = qu_node_content(node);
        if(*data) {
            *dlen = strlen(*data);
            return 1;
        }
    }
    return 0;
}

void qu_vars_init(struct qu_config_context *ctx) {
	ctx->variables.root = NULL;
}
