#include "emitter.h"

int qu_emit_init(qu_emit_context *ctx, FILE *stream) {
    // Default settings
    ctx->min_indent = 4;
    ctx->width = 80;
    ctx->canonical_tags = 0;
    ctx->always_quote = 0;
    ctx->always_flow = 0;

    // State
    ctx->stream = stream;
    ctx->flow_level = 0;
    ctx->cur_indent = 0;
    return 0;
}

int qu_emit_free(qu_emit_context *ctx) {
    return 0;
}

int qu_emit_comment(qu_emit_context *ctx, int flags, char *data, int len) {
    // TODO(tailhook) implement comment reformatting
    if(len < 0) {
        fprintf(ctx->stream, "# %s\n", data);
    } else {
        fprintf(ctx->stream, "# %.*s\n", len, data);
    }
    return 0;
}

int qu_emit_whitespace(qu_emit_context *ctx, int kind, int count) {
    // TODO(tailhook) implement checks
    switch(kind) {
    case QU_WS_SPACE:
        for(int i = 0; i < count; ++i) {
            fputc(' ', ctx->stream);
        };
        break;
    case QU_WS_ENDLINE:
        for(int i = 0; i < count; ++i) {
            fputc('\n', ctx->stream);
        };
        break;
    case QU_WS_INDENT:
        for(int i = 0; i < count; ++i) {
            fputc(' ', ctx->stream);
        };
        break;
    }
    return 0;
}
