#include "struct.h"
#include "context.h"

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

struct qu_config_struct *qu_struct_new(struct qu_context *ctx,
    struct qu_config_struct *parent)
{
    struct qu_config_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_config_struct));
    qu_struct_init(self);
    self->parent = parent;
    TAILQ_INSERT_TAIL(&parent->children, self, lst);
    return self;
}
