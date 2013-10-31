#include "guard.h"
#include "util/print.h"
#include "context.h"

struct qu_guard *qu_guard_new(struct qu_context *ctx, const char *name) {
    struct qu_guard *self = obstack_alloc(&ctx->parser.pieces,
                sizeof(struct qu_guard));
    self->condition = name;
    return self;
}

void qu_guard_print_open(struct qu_context *ctx, struct qu_guard *guard) {
    if(!guard)
        return;
    qu_code_print(ctx,
        "#if ${cond}\n"
        , "cond", guard->condition
        , NULL);
}

void qu_guard_print_close(struct qu_context *ctx, struct qu_guard *guard) {
    if(!guard)
        return;
    qu_code_print(ctx,
        "#endif  /* ${cond} */\n"
        , "cond", guard->condition
        , NULL);
}

