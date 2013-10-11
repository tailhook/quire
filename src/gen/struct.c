#include "struct.h"
#include "context.h"
#include "types/types.h"
#include "util/print.h"

static void qu_struct_init(struct qu_config_struct *self) {
    self->parent = NULL;
    TAILQ_INIT(&self->children);
}

struct qu_config_struct *qu_struct_new_root(struct qu_context *ctx) {
    struct qu_config_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_config_struct));
    qu_struct_init(self);
    return self;
}

struct qu_config_struct *qu_struct_substruct(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name)
{
    struct qu_config_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_config_struct));
    qu_struct_init(self);
    self->parent = parent;
    if(parent->path) {
        self->path = qu_template_alloc(ctx, "${parent}.${name}",
            "parent", parent->path,
            "name", name,
            NULL);
    } else {
        self->path = name;
    }
    struct qu_struct_member *mem = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_struct_member));
    mem->name = name;
    mem->p.str = self;
    TAILQ_INSERT_TAIL(&parent->children, mem, lst);
    return self;
}

void qu_struct_add_option(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name, struct qu_option *option)
{
    struct qu_struct_member *mem = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_struct_member));
    mem->name = name;
    mem->p.opt = option;
    TAILQ_INSERT_TAIL(&parent->children, mem, lst);
    if(parent->path) {
        option->path = qu_template_alloc(ctx, "${parent}.${name}",
            "parent", parent->path,
            "name", name,
            NULL);
    } else {
        option->path = name;
    }
}
