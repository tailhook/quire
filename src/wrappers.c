#include "wrappers.h"
#include "maputil.h"
#include "include.h"

int qu_process_includes(qu_parse_context *ctx, int flags) {
    int rc;
    ctx->has_jmp = 1;
    if(!(rc = setjmp(ctx->errjmp))) {
        _qu_process_includes(ctx, flags);
    } else {
        ctx->has_jmp = 0;
        return rc;
    }
    ctx->has_jmp = 0;
    return 0;
}

int qu_merge_maps(qu_parse_context *ctx, int flags) {
    int rc;
    ctx->has_jmp = 1;
    if(!(rc = setjmp(ctx->errjmp))) {
        _qu_merge_maps(ctx, flags);
    } else {
        ctx->has_jmp = 0;
        return rc;
    }
    ctx->has_jmp = 0;
    return 0;
}
