#include "context.h"

void qu_context_init(qu_context_t *ctx, jmp_buf *jmp) {
    TAILQ_INIT(&ctx->arrays);
    TAILQ_INIT(&ctx->mappings);
	ctx->parser.errjmp = jmp;
    qu_parser_init(&ctx->parser);
    qu_fwdecl_init(ctx);
    qu_cli_init(ctx);
}
