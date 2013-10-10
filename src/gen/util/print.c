#include <stdarg.h>
#include <assert.h>

#include "print.h"
#include "../context.h"

#define slice_equal(start, len, val) \
    ((len) == (int)strlen((val)) && !strncmp((val), (start), (len)))

static void qu_code_growval(const char *val, struct obstack *buf,
    const char *fmt)
{
	if(fmt == NULL) {
		obstack_grow(buf, val, strlen(val));
	} else {
		if(fmt[1] == 'q') {
			obstack_1grow(buf, '"');
            for(const char *c = val; *c; ++c) {
                if(*c < 32) {
                    char tbuf[8];
                    int tlen;
                    switch(*c) {
                    case '\r': tlen = sprintf(tbuf, "\\r"); break;
                    case '\n': tlen = sprintf(tbuf, "\\n"); break;
                    case '\t': tlen = sprintf(tbuf, "\\t"); break;
                    default: tlen = sprintf(tbuf, "\\x%02x", *c); break;
                    }
                    obstack_grow(buf, tbuf, tlen);
                    continue;
                } else if(*c == '\\' || *c == '"') {
                    obstack_1grow(buf, '\\');
                    obstack_1grow(buf, *c);
                } else {
                    obstack_1grow(buf, *c);
                }
            }
			obstack_1grow(buf, '"');
		}
	}
}

static const char *qu_template_grow_va(struct qu_context *ctx,
    const char *template, va_list args)
{
    const char *c;
    for(c = template; *c; ++c) {
        if(*c == '`') {
            obstack_1grow(&ctx->parser.pieces, '"');
		} else if(*c == '$') {
			assert (*(c+1) == '{');
            const char *varend = strchr(c+1, '}');
            const char *var = c+2;
			const char *fmt = strchr(c+1, ':');
			if(fmt && fmt > varend)
				fmt = NULL;
            int var_len = (fmt ? fmt : varend) - var;
            assert(varend);
            if(slice_equal(var, var_len, "pref"))
                qu_code_growval(ctx->prefix, &ctx->parser.pieces, fmt);
            else if(slice_equal(var, var_len, "mpref"))
                qu_code_growval(ctx->macroprefix, &ctx->parser.pieces, fmt);
            else {
                va_list vars;
                va_copy (vars, args);
                while(1) {
                    const char *name = va_arg(vars, char *);
                    if(!name)
                        break;
                    const char *value = va_arg(vars, char *);
                    if(slice_equal(var, var_len, name)) {
                        qu_code_growval(value, &ctx->parser.pieces, fmt);
                    }
                }
                va_end (vars);
            }
            c = varend;
        } else {
            obstack_1grow(&ctx->parser.pieces, *c);
        }
    }
}

const char *qu_template_alloc(struct qu_context *ctx,
    const char *template, ...)
{
    va_list args;
    va_start (args, template);
    obstack_blank(&ctx->parser.pieces, 0);

    qu_template_grow_va(ctx, template, args);

    va_end (args);
    obstack_1grow(&ctx->parser.pieces, 0);
    return obstack_finish(&ctx->parser.pieces);
}

void qu_template_grow(struct qu_context *ctx,
    const char *template, ...)
{
    va_list args;
    va_start (args, template);

    qu_template_grow_va(ctx, template, args);

    va_end (args);
}

void qu_code_print(struct qu_context *ctx, const char *template, ...) {
    va_list args;
    va_start (args, template);
    obstack_blank(&ctx->parser.pieces, 0);

    qu_template_grow_va(ctx, template, args);

    va_end (args);
    int clen = obstack_object_size(&ctx->parser.pieces);
    const char *data = obstack_finish(&ctx->parser.pieces);
    fwrite(data, 1, clen, ctx->out);
    obstack_free(&ctx->parser.pieces, data);
}
