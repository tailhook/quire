#include <malloc.h>
#include <setjmp.h>
#include <errno.h>

#include "api.h"
#include "context.h"

struct qu_config_context *qu_config_parser(jmp_buf *jmp) {
	struct qu_config_context *ctx = malloc(sizeof(struct qu_config_context));
	if(!ctx) {
		longjmp(*jmp, -errno);
	}
	qu_config_context_init(ctx, jmp);
	return ctx;
}
void qu_config_parser_free(struct qu_config_context *ctx) {
	qu_config_context_free(ctx);
	free(ctx);
}
