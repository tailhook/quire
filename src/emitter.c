#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

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
    ctx->map_start = 0;
    ctx->seq_start = 0;
    return 0;
}

int qu_emit_done(qu_emit_context *ctx) {
    if(!ctx->line_start) {
        fputc('\n', ctx->stream);
    }
    fflush(ctx->stream);
    return 0;
}

static int _space_check(qu_emit_context *ctx) {
    if(!ctx->pending_newline) {
        fputc('\n', ctx->stream);
        ctx->pending_newline = -1;
        ctx->line_start = 1;
        ctx->need_space = 0;
    } else if(ctx->need_space) {
        fputc(' ', ctx->stream);
        ctx->line_start = 0;
        ctx->need_space = 0;
    }
    if(ctx->line_start) {
        for(int i = 0; i < ctx->indent_levels[ctx->cur_indent]; ++i) {
            fputc(' ', ctx->stream);
        }
        ctx->line_start = 0;
        ctx->doc_start = 0;
    }
    return 0;
}

int qu_emit_comment(qu_emit_context *ctx, int flags, const char *data, int len) {
    // TODO(tailhook) implement comment reformatting
    if(len < 0) {
		fprintf(ctx->stream, "# %s\n", data);
    } else {
		if(len == 0) {
			/*  Empty line for beauty  */
			fprintf(ctx->stream, "#\n");
		} else {
			fprintf(ctx->stream, "# %.*s\n", len, data);
		}
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

int qu_emit_opcode(qu_emit_context *ctx, const char *tag, const char *anchor, int code) {
    switch(code) {
        case QU_EMIT_MAP_START:
            if(!ctx->doc_start) {
                int oi = ctx->indent_levels[ctx->cur_indent];
                ctx->cur_indent += 1;
                ctx->indent_levels[ctx->cur_indent] = oi + ctx->min_indent;
                if(!ctx->line_start) {
                    if(ctx->seq_item) {
                        ctx->pending_newline += 1;
                    } else {
                        ctx->pending_newline = 0;
                    }
                }
            }
            if(tag) {
                _space_check(ctx);
                fprintf(ctx->stream, "%s", tag);
                ctx->need_space = 1;
                ctx->pending_newline = 0;
            }
            ctx->map_start = 1;
            break;
        case QU_EMIT_MAP_END:
            if(ctx->map_start) {
                if(ctx->need_space)
                    fputc(' ', ctx->stream);
                fputs("{}", ctx->stream);
                ctx->need_space = 1;
                ctx->pending_newline = 0;
                ctx->line_start = 0;
            }
            ctx->cur_indent -= 1;
            break;
        case QU_EMIT_MAP_KEY:
            ctx->map_start = 0;
            ctx->seq_start = 0;
            _space_check(ctx);
            fprintf(ctx->stream, "?");
            ctx->need_space = 1;
            ctx->pending_newline = 1;
            ctx->line_start = 0;
            break;
        case QU_EMIT_MAP_VALUE:
            _space_check(ctx);
            fprintf(ctx->stream, ":");
            ctx->need_space = 1;
            ctx->pending_newline = 1;
            ctx->line_start = 0;
            break;
        case QU_EMIT_SEQ_ITEM:
            ctx->map_start = 0;
            ctx->seq_start = 0;
            ctx->seq_item = 1;
            _space_check(ctx);
            fprintf(ctx->stream, "-");
            ctx->need_space = 1;
            ctx->pending_newline = 1;
            ctx->line_start = 0;
            break;
        case QU_EMIT_SEQ_START:
            ctx->seq_start = 1;
            if(!ctx->line_start) {
                ctx->pending_newline = 0;
            }
            break;
        case QU_EMIT_SEQ_END:
            if(ctx->seq_start) {
                if(ctx->need_space)
                    fputc(' ', ctx->stream);
                fputs("[]", ctx->stream);
                ctx->need_space = 1;
                ctx->pending_newline = 0;
                ctx->line_start = 0;
            }
            break;
        default:
            assert(0);
    }
    ctx->doc_start = 0;
    return 0;
}

int qu_emit_scalar(qu_emit_context *ctx, const char *tag, const char *anchor, int kind,
    const char *data, int len) {
    int need_quotes;
    if(!len || !data || !*data) {
        if(ctx->doc_start) {
            // document is fully null
            ctx->doc_start = 0;
            return 0;
        }
        if(ctx->line_start) {  // force tilde
            fputc('~', ctx->stream);
            ctx->line_start = 0;
            ctx->pending_newline = 0;
            _space_check(ctx);
            return 0;
        }
        // force newline
        fputc('\n', ctx->stream);
        ctx->pending_newline = -1;
        ctx->line_start = 1;
        ctx->need_space = 0;

        ctx->map_start = 0;
        ctx->seq_start = 0;
        return 0;
    }
    if(tag) {
        _space_check(ctx);
        fprintf(ctx->stream, "%s", tag);
        ctx->need_space = 1;
    }
    _space_check(ctx);
    ctx->map_start = 0;
    ctx->seq_start = 0;
    ctx->seq_item = 0;

    if(len < 0)
        len = strlen(data);
    switch(kind) {
        case QU_STYLE_AUTO:
            need_quotes = 0;
            for(int i = 0; i < len; ++i) {
                if(!isprint(data[i])) {
                    need_quotes = 1;
                    break;
                }
            }
            if(need_quotes) {
                fputc ('"', ctx->stream);
                for (; len > 0; --len, ++data) {
                    switch(*data) {
                    case '\n':
                        fprintf(ctx->stream, "\\n");
                        break;
                    case '\r':
                        fprintf(ctx->stream, "\\r");
                        break;
                    case '\\':
                    case '\"':
                        fprintf(ctx->stream, "\\%c", *data);
                        break;
                    default:
                        if(isprint(*data)) {
                            fputc(*data, ctx->stream);
                        } else {
                            fprintf(ctx->stream, "\\x%02x", *data);
                        }
                    }
                }
                fprintf (ctx->stream, "\"");
            } else {
                fprintf(ctx->stream, "%.*s", len, data);
            }
            break;
        case QU_STYLE_PLAIN:
            fprintf(ctx->stream, "%.*s", len, data);
            break;
        default:
            assert(0);
    }
    ctx->pending_newline -= 1;
    ctx->line_start = 0;
    return 0;
}

int qu_emit_printf(qu_emit_context *ctx, const char *tag, const char *anchor, int kind,
    const char *format, ...) {
    if(tag) {
        _space_check(ctx);
        fprintf(ctx->stream, "%s", tag);
        ctx->need_space = 1;
    }
    _space_check(ctx);
    ctx->map_start = 0;
    ctx->seq_start = 0;
    ctx->seq_item = 0;

    switch(kind) {
        case QU_STYLE_AUTO:  // TODO(tailhook) scan scalar
        case QU_STYLE_PLAIN: {
            va_list args;
            va_start(args, format);
            vfprintf(ctx->stream, format, args);
            va_end(args);
            }; break;
        default:
            assert(0);
    }
    ctx->pending_newline -= 1;
    ctx->line_start = 0;
    return 0;
}

int qu_emit_alias(qu_emit_context *ctx, const char *name) {
    _space_check(ctx);
    fprintf(ctx->stream, "%s", name);
    ctx->pending_newline -= 1;
    ctx->line_start = 0;
    return 0;
}
