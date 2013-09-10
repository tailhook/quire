#include "wrappers.h"
#include "maputil.h"
#include "include.h"

int qu_process_includes(qu_parse_context *ctx, int flags) {
    int rc;
	if(!ctx->errjmp) {
		ctx->errjmp = &ctx->errjmp_buf;
		if(!(rc = setjmp(ctx->errjmp_buf))) {
			_qu_process_includes(ctx, flags);
		} else {
			ctx->errjmp = NULL;
			return rc;
		}
		ctx->errjmp = NULL;
	} else {
		_qu_process_includes(ctx, flags);
	}
    return 0;
}

int qu_merge_maps(qu_parse_context *ctx, int flags) {
    int rc;
	if(!ctx->errjmp) {
		ctx->errjmp = &ctx->errjmp_buf;
		if(!(rc = setjmp(ctx->errjmp_buf))) {
			_qu_merge_maps(ctx, flags);
		} else {
			ctx->errjmp = NULL;
			return rc;
		}
		ctx->errjmp = NULL;
	} else {
		_qu_merge_maps(ctx, flags);
	}
    return 0;
}
