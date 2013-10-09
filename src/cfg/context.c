#include <setjmp.h>

#include "context.h"

void qu_config_context_init(struct qu_config_context *ctx, jmp_buf *jmp) {
	ctx->parser.errjmp = jmp;
	qu_parser_init(&ctx->parser);
	qu_vars_init(ctx);
}
void qu_config_context_free(struct qu_config_context *ctx) {
	qu_parser_free(&ctx->parser);
	/*  Everhing else will be freed with obstack in ctx->parser  */
}
