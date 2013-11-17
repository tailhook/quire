#include "context.h"

void qu_context_init(qu_context_t *ctx, jmp_buf *jmp) {
    qu_err_init(&ctx->errbuf, jmp);
    ctx->err = &ctx->errbuf;
    qu_parser_init(&ctx->parser, ctx->err);
    qu_fwdecl_init(ctx);
    qu_cli_init(ctx);
    qu_classes_init(ctx);
    qu_include_init(ctx);
}
