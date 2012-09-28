#include "wrappers.h"
#include "maputil.h"

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
