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
    qu_vartype_t type;
    union qu_variable_data {
        struct {
            struct qu_node_s *node;
        } anchor;
        struct {
            char *value;
            int value_len;
        } string;
        struct {
            long value;
        } integer;
    } data;
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
    (*var)->type = QU_VAR_STRING;
    (*var)->data.string.value = obstack_copy0(&ctx->parser.pieces, data, dlen);
    (*var)->data.string.value_len = dlen;
    return 0;
}

int qu_set_integer(qu_config_context *ctx, const char *name, long value) {
    struct qu_variable **var = qu_var_find(&ctx->variables, name, strlen(name));
    if(!*var)
        *var = qu_var_new(ctx, name);
    (*var)->type = QU_VAR_INTEGER;
    (*var)->data.integer.value = value;
    return 0;
}

int qu_get_string(qu_config_context *ctx, const char *name,
    const char **data, int *dlen) {
    return qu_get_string_len(ctx, name, strlen(name), data, dlen);
}

int qu_get_string_len(qu_config_context *ctx, const char *name, int nlen,
    const char **data, int *dlen) {
    char *buf;
    struct qu_variable **ptr = qu_var_find(&ctx->variables, name, nlen);
    struct qu_variable *var = *ptr;
    if(var) {
        switch(var->type) {
            case QU_VAR_ANCHOR:
                *data = qu_node_content(var->data.anchor.node);
                *dlen = strlen(*data);
                return 0;
            case QU_VAR_STRING:
                *data = var->data.string.value;
                *dlen = var->data.string.value_len;
                return 0;
            case QU_VAR_INTEGER:
                buf = obstack_alloc(&ctx->parser.pieces, 24);
                *data = buf;
                *dlen = sprintf(buf, "%ld", var->data.integer.value);
                return 0;
            default:
                return -1; // maybe will fix this
        };
    }
    qu_ast_node *node = qu_find_anchor(&ctx->parser, name, strlen(name));
    if(node) {
        *data = qu_node_content(node);
        if(*data) {
            *dlen = strlen(*data);
            return 0;
        }
    }
    return -1;
}

static void qu_print_var(struct qu_variable *var) {
    if(var->left) {
        qu_print_var(var->left);
    }
    switch(var->type) {
        case QU_VAR_STRING:
            printf("%s=%.*s\n", var->name,
                var->data.string.value_len,
                var->data.string.value);
            break;
        case QU_VAR_INTEGER:
            printf("%s=%ld\n", var->name, var->data.integer.value);
            break;
        default: break;
    };
    if(var->right) {
        qu_print_var(var->right);
    }
}

int qu_print_variables(qu_config_context *ctx) {
    qu_print_var(ctx->variables.root);
    return 0;
}

void qu_vars_init(struct qu_config_context *ctx) {
	ctx->variables.root = NULL;
}
