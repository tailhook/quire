#include "types.h"
#include "../context.h"
#include "../../error.h"
#include "../../yaml/access.h"
#include "../classes/classes.h"
#include "../../quire_int.h"

struct qu_class **qu_class_find(struct qu_class_index *idx, const char *name) {
    struct qu_class **node = &idx->root;
    while(*node) {
        int rc = strcmp(name, (*node)->name);
        if(rc == 0)
            return node;
        if(rc < 0)
            node = &(*node)->left;
        else if(rc > 0)
            node = &(*node)->right;
    }
    return node;
}

struct qu_class *qu_class_new(struct qu_context *ctx, const char *name,
    qu_ast_node *definition)
{
    if(!definition->tag) {
        qu_err_node_error(ctx->err, definition,
            "Custom type must be tagged");
        return NULL;
    }
    struct qu_class_vptr *vp = qu_class_get_vptr(definition->tag);
    if(!vp) {
        qu_err_node_error(ctx->err, definition,
            "Unsupported custom type kind \"%s\"", definition->tag);
        return NULL;
    }
    struct qu_class *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class));
    self->name = name;
    self->start_token = definition->tag_token;
    self->vp = vp;
    self->classdata = NULL;
    self->left = NULL;
    self->right = NULL;
    self->has_default = 0;
    return self;
}


void qu_special_types(struct qu_context *ctx, qu_ast_node *typesnode) {
    if(typesnode->kind != QU_NODE_MAPPING)
        return;
    qu_map_member *item;
    TAILQ_FOREACH(item, &typesnode->val.map_index.items, lst) {
        struct qu_class **cls;
        const char *cname = qu_node_content(item->key);
        cls = qu_class_find(&ctx->class_index, cname);
        if(*cls) {
            qu_err_node_error(ctx->err, item->key, "Duplicate type");
            continue;
        }
        *cls = qu_class_new(ctx, cname, item->value);
        if(!*cls)
            continue;  /*  Error is already reported  */
        (*cls)->node = item->value;
        (*cls)->vp->init(ctx, (*cls), item->value);
    }
}

void qu_classes_init(struct qu_context *ctx) {
    ctx->class_index.root = NULL;
}

struct qu_class *qu_class_get(struct qu_context *ctx, const char *name) {
    return *qu_class_find(&ctx->class_index, name);
}

