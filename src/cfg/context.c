#include <setjmp.h>

#include "context.h"
#include "../quire_int.h"

void qu_config_context_init(struct qu_config_context *ctx,
    struct qu_config_head *target, jmp_buf *jmp)
{
    qu_err_init(&ctx->errbuf, jmp);
    ctx->err = &ctx->errbuf;
    ctx->alloc = &target->pieces;
    qu_parser_init(&ctx->parser, ctx->err);
    ctx->vars = qu_anchors_frame(&ctx->parser.pieces, &ctx->parser, NULL);
    ctx->vars = qu_vars_frame(&ctx->parser.pieces, ctx->vars);
}
void qu_config_context_free(struct qu_config_context *ctx) {
    qu_parser_free(&ctx->parser);
    /*  Everhing else will be freed with obstack in ctx->parser  */
}
