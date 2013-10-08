#include "struct.h"
#include "context.h"

struct qu_config_struct *qu_struct_new_root(struct qu_context *ctx) {
    struct qu_config_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_config_struct));
    self->parent = NULL;
    LIST_INIT(&self->children);
    return self;
}
