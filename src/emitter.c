#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "emitter.h"


int qu_emit_init(qu_emit_context *ctx, FILE *stream) {
    // Default settings
    ctx->min_indent = 2;
    ctx->width = 80;
    ctx->canonical_tags = 0;
    ctx->always_quote = 0;
    ctx->always_flow = 0;

    // State
    ctx->stream = stream;
    ctx->flow_level = 0;
    ctx->cur_indent = 0;
    ctx->indent_levels[ctx->cur_indent] = 0;
    ctx->doc_start = 1;
    ctx->line_start = 1;
    ctx->need_space = 0;
    ctx->pending_newline = -1;
    return 0;
}

int qu_emit_done(qu_emit_context *ctx) {
    if(!ctx->line_start)
        putc('\n', ctx->stream);
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
        ctx->need_space = 0;
        break;
    case QU_WS_ENDLINE:
        for(int i = 0; i < count; ++i) {
            fputc('\n', ctx->stream);
        };
        ctx->pending_newline = -1;
        break;
    case QU_WS_INDENT:
        for(int i = 0; i < count; ++i) {
            fputc(' ', ctx->stream);
        };
        break;
    }
    return 0;
}

int qu_emit_opcode(qu_emit_context *ctx, char *tag, char *anchor, int code) {
    switch(code) {
        case QU_EMIT_MAP_START:
            if(!ctx->doc_start) {
                int oi = ctx->indent_levels[ctx->cur_indent];
                ctx->cur_indent += 1;
                ctx->indent_levels[ctx->cur_indent] = oi + ctx->min_indent;
                if(!ctx->line_start) {
                    ctx->pending_newline = 0;
                }
            }
            break;
        case QU_EMIT_MAP_END:
            break;
        case QU_EMIT_MAP_KEY:
            printf("?");
            ctx->need_space = 1;
            ctx->pending_newline = 1;
            ctx->line_start = 0;
            break;
        case QU_EMIT_MAP_VALUE:
            printf(":");
            ctx->need_space = 1;
            ctx->pending_newline = 1;
            ctx->line_start = 0;
            break;
        case QU_EMIT_SEQ_START:
            break;
        case QU_EMIT_SEQ_END:
            break;
        default:
            assert(0);
    }
    ctx->doc_start = 0;
    return 0;
}

int qu_emit_scalar(qu_emit_context *ctx, char *tag, char *anchor, int kind,
    char *data, int len) {
    if(len >= 0) {
        return qu_emit_printf(ctx, tag, anchor, kind, "%.*s", len, data);
    } else {
        return qu_emit_printf(ctx, tag, anchor, kind, "%s", data);
    }
}

int qu_emit_printf(qu_emit_context *ctx, char *tag, char *anchor, int kind,
    char *format, ...) {
    if(!ctx->pending_newline) {
        putc('\n', ctx->stream);
        ctx->pending_newline = -1;
        ctx->line_start = 1;
    } else if(ctx->need_space) {
        putc(' ', ctx->stream);
        ctx->line_start = 0;
    }
    if(ctx->line_start) {
        for(int i = 0; i < ctx->indent_levels[ctx->cur_indent]; ++i) {
            putc(' ', ctx->stream);
        }
        ctx->line_start = 0;
        ctx->doc_start = 0;
    }

    switch(kind) {
        case QU_STYLE_AUTO:  // TODO(tailhook) scan scalar
        case QU_STYLE_PLAIN: {
            va_list args;
            va_start(args, format);
            vprintf(format, args);
            va_end(args);
            }; break;
        default:
            assert(0);
    }
    ctx->pending_newline -= 1;
    return 0;
}
