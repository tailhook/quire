#include "types.h"
#include "../context.h"
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
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, definition->start_token,
            "Custom type must be tagged");
    }
    struct qu_class_vptr *vp = qu_class_get_vptr(
        (char *)definition->tag->data, definition->tag->bytelen);
    if(!vp) {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, definition->tag,
            "Unsupported custom type kind");
    }
    struct qu_class *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class));
    self->name = name;
    self->start_token = definition->tag;
    self->vp = vp;
    self->classdata = NULL;
    self->left = NULL;
    self->right = NULL;
    vp->init(ctx, self);
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
        if(!*cls) {
            LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, item->key->start_token,
                "Duplicate type");
        }
        *cls = qu_class_new(ctx, cname, item->value);
    }
}
