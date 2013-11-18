#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vars.h"
#include "../yaml/access.h"
#include "../yaml/anchors.h"

#define min(a, b) ((a) > (b) ? (b) : (a))

typedef int (*qu_var_resolver)(struct qu_var_frame *frame,
    const char *name, int nlen,
    const char **result, int *rlen);

struct qu_variable {
    struct qu_variable *left;
    struct qu_variable *right;
    char *name;
    int name_len;
    char *value;
    int value_len;
};

struct qu_var_frame {
    qu_var_resolver resolver;
    struct qu_var_frame *parent;
};

struct qu_var_frame_tree {
    struct qu_var_frame frame;
    struct qu_variable *vars;
};

struct qu_var_frame_anchors {
    struct qu_var_frame frame;
    struct qu_parser *ctx;
};

struct qu_var_frame_node {
    struct qu_var_frame frame;
    qu_ast_node *node;
};

static int qu_string_var(struct qu_var_frame *frame,
    const char *name, int name_len, const char **data, int *dlen);

static struct qu_variable **qu_var_find(struct qu_var_frame *frame,
    const char *name, int nlen)
{
    assert(frame->resolver == qu_string_var);
    struct qu_variable **node = &((struct qu_var_frame_tree *)frame)->vars;
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

static int qu_string_var(struct qu_var_frame *frame,
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
}


static int qu_anchor_var(struct qu_var_frame *frame, const char *name, int nlen,
    const char **data, int *dlen)
{
    qu_ast_node *node = qu_find_anchor(
        ((struct qu_var_frame_anchors *)frame)->ctx, name, nlen);
    if(node) {
        *data = qu_node_content(node);
        if(*data) {
            *dlen = strlen(*data);
            return 1;
        }
    }
    return 0;
}

static int qu_node_var(struct qu_var_frame *frame, const char *name, int nlen,
    const char **data, int *dlen)
{
    qu_ast_node *node = qu_map_get_len(
        ((struct qu_var_frame_node *)frame)->node, name, nlen);
    if(node) {
        *data = qu_node_content(node);
        if(*data) {
            *dlen = strlen(*data);
            return 1;
        }
    }
    return 0;
}

struct qu_var_frame *qu_vars_frame(struct obstack *buf,
    struct qu_var_frame *parent)
{
    struct qu_var_frame_tree *self;
    self = obstack_alloc(buf, sizeof(struct qu_var_frame_tree));
    self->frame.resolver = qu_string_var;
    self->frame.parent = parent;
    self->vars = NULL;
    return (struct qu_var_frame *)self;
}

struct qu_var_frame *qu_anchors_frame(struct obstack *buf,
    struct qu_parser *ctx, struct qu_var_frame *parent)
{
    struct qu_var_frame_anchors *self;
    self = obstack_alloc(buf, sizeof(struct qu_var_frame_anchors));
    self->frame.resolver = qu_anchor_var;
    self->frame.parent = parent;
    self->ctx = ctx;
    return (struct qu_var_frame *)self;
}

struct qu_var_frame *qu_node_frame(struct obstack *buf,
    qu_ast_node *node, struct qu_var_frame *parent)
{
    struct qu_var_frame_node *self;
    self = obstack_alloc(buf, sizeof(struct qu_var_frame_node));
    self->frame.resolver = qu_node_var;
    self->frame.parent = parent;
    self->node = node;
    return (struct qu_var_frame *)self;
}

int qu_var_get_string(struct qu_var_frame *frame,
    const char *name, int name_len, const char **data, int *dlen) {
    while(frame) {
        if(frame->resolver(frame, name, name_len, data, dlen))
            return 1;
        frame = frame->parent;
    }
    return 0;
}
