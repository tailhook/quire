#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vars.h"
#include "../yaml/access.h"
#include "../yaml/anchors.h"

#define min(a, b) ((a) > (b) ? (b) : (a))

struct qu_variable {
    struct qu_variable *left;
    struct qu_variable *right;
    char *name;
    int name_len;
    char *value;
    int value_len;
};

struct qu_var_frame {
    struct qu_var_frame *parent;
    struct qu_variable *vars;
};

static struct qu_variable **qu_var_find(struct qu_var_frame *frame,
    const char *name, int nlen)
{
    struct qu_variable **node = &frame->vars;
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

void qu_var_set_string(struct obstack *buf, struct qu_var_frame *frame,
    const char *name, int nlen, const char *data, int dlen)
{
    struct qu_variable **var = qu_var_find(frame, name, nlen);
    struct qu_variable *v = *var;
    if(!v) {
        *var = v = obstack_alloc(buf, sizeof(struct qu_variable));
        v->left = NULL;
        v->right = NULL;
        v->name_len = nlen;
        v->name = obstack_copy0(buf, name, nlen);
    }
    v->value = obstack_copy0(buf, data, dlen);
    v->value_len = dlen;
    printf("KEY ``%.*s'' VAL ``%.*s''\n", nlen, name, dlen, data);
}

int qu_string_var(struct qu_var_frame *frame,
    const char *name, int name_len, const char **data, int *dlen)
{
    struct qu_variable **ptr = qu_var_find(frame, name, name_len);
    if(*ptr) {
        *data = (*ptr)->value;
        *dlen = (*ptr)->value_len;
        return 1;
    }
    return 0;
}

int qu_anchor_var(struct qu_parser *ctx, const char *name, int nlen,
    const char **data, int *dlen)
{
    qu_ast_node *node = qu_find_anchor(ctx, name, nlen);
    if(node) {
        *data = qu_node_content(node);
        if(*data) {
            *dlen = strlen(*data);
            return 1;
        }
    }
    return 0;
}

struct qu_var_frame *qu_var_new_frame(struct obstack *buf,
    struct qu_var_frame *parent)
{
    struct qu_var_frame *self;
    self = obstack_alloc(buf, sizeof(struct qu_var_frame));
    self->parent = parent;
    self->vars = NULL;
    return self;
}
