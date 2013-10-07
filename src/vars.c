#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vars.h"
#include "../yaml/access.h"

static int find_value(qu_variable_t *var, char *name, int nlen,
    qu_variable_t **node) {
    qu_variable_t *parent;
    qu_variable_t *cur = var;
    while(1) {
        int res = strncmp(name, cur->name, nlen);
        if(!res) {
            res = nlen - strlen(cur->name);
        }
        if(!res) {
            *node = cur;
            return 0;
        }
        if(res < 0) {
            parent = cur;
            cur = cur->left;
            if(!cur) {
                *node = parent;
                return -1;
            }
        } else { // res > 0
            parent = cur;
            cur = cur->right;
            if(!cur) {
                *node = parent;
                return 1;
            }
        }
    }
    assert(0);
}

static qu_variable_t *new_variable(qu_parse_context *ctx, char *name) {
    qu_variable_t *var = obstack_alloc(&ctx->pieces,
        sizeof(qu_variable_t) + strlen(name) + 1);
    var->left = NULL;
    var->right = NULL;
    var->name = ((char *)var) + sizeof(qu_variable_t);
    var->name_len = strlen(name);
    memcpy(var->name, name, var->name_len);
    var->name[var->name_len] = 0;
    return var;
}

static qu_variable_t *find_and_set(qu_parse_context *ctx, char *name) {
    qu_variable_t *var;
    if(!ctx->variables) {
        ctx->variables = var = new_variable(ctx, name);
    } else {
        qu_variable_t *node;
        int rel = find_value(ctx->variables, name, strlen(name), &node);
        if(rel == 1) {
            var = node->right = new_variable(ctx, name);
        } else if(rel == -1) {
            var = node->left = new_variable(ctx, name);
        } else {
            assert(0);
        }
    }
    return var;
}

int qu_set_string(qu_parse_context *ctx, char *name, char *data) {
	int dlen = strlen(data);
    qu_variable_t *var = find_and_set(ctx, name);
    if(!var) return -1;
    var->type = QU_VAR_STRING;
    var->data.string.value = obstack_copy0(&ctx->pieces, data, dlen);
    var->data.string.value_len = dlen;
    return 0;
}

int qu_set_integer(qu_parse_context *ctx, char *name, long value) {
    qu_variable_t *var = find_and_set(ctx, name);
    if(!var) return -1;
    var->type = QU_VAR_INTEGER;
    var->data.integer.value = value;
    return 0;
}

int qu_get_string(qu_parse_context *ctx, char *name,
    char **data, int *dlen) {
    qu_variable_t *var;
    if(!ctx->variables) return -1;
    int val = find_value(ctx->variables, name, strlen(name), &var);
    if(!val) {
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
                *data = obstack_alloc(&ctx->pieces, 24);
                *dlen = sprintf(*data, "%ld", var->data.integer.value);
                return 0;
            default:
                return -1; // maybe will fix this
        };
    }
    return -1;
}

static void print_var(qu_variable_t *var) {
    if(var->left) {
        print_var(var->left);
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
        print_var(var->right);
    }
}

int qu_print_variables(qu_parse_context *ctx) {
    print_var(ctx->variables);
    return 0;
}

void _qu_insert_anchor(qu_parse_context *ctx,
    unsigned char *name, int namelen, qu_ast_node *node) {
    char *sname = obstack_copy0(&ctx->pieces, name, namelen);
    qu_variable_t *var = find_and_set(ctx, sname);
    assert(var);
    var->type = QU_VAR_ANCHOR;
    var->data.anchor.node = node;
}

qu_ast_node *_qu_find_anchor(qu_parse_context *ctx,
    unsigned char *name, int namelen) {
    qu_variable_t *node;
    int rel = find_value(ctx->variables, (char*)name, namelen, &node);
    if(rel != 0)
        return NULL;
    if(node->type == QU_VAR_ANCHOR) {
        return node->data.anchor.node;
    }
    return NULL;
}

