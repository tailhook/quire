#include <setjmp.h>

#include "context.h"
#include "../quire_int.h"

void qu_config_context_init(struct qu_config_context *ctx,
    struct qu_config_head *target, jmp_buf *jmp)
{
    ctx->parser.errjmp = jmp;
    ctx->alloc = &target->pieces;
    qu_parser_init(&ctx->parser);
    qu_vars_init(ctx);
}
void qu_config_context_free(struct qu_config_context *ctx) {
    qu_parser_free(&ctx->parser);
    /*  Everhing else will be freed with obstack in ctx->parser  */
}
