#include "types.h"
#include "../context.h"

struct qu_option *qu_option_new(struct qu_context *ctx,
    struct qu_option_vptr *vp)
{
    struct qu_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_option));
    self->vp = vp;
    return self;
}
