#include <stdarg.h>
#include <assert.h>

#include "print.h"
#include "../context.h"

#define slice_equal(start, len, val) \
    ((len) == (int)strlen((val)) && !strncmp((val), (start), (len)))

void qu_code_putval(const char *val, FILE *out, const char *fmt) {
	if(fmt == NULL) {
		fputs(val, out);
	} else {
		if(fmt[1] == 'q') {
			fputc('"', out);
            for(const char *c = val; *c; ++c) {
                if(*c < 32) {
                    switch(*c) {
                    case '\r': fprintf(out, "\\r"); break;
                    case '\n': fprintf(out, "\\n"); break;
                    case '\t': fprintf(out, "\\t"); break;
                    default: fprintf(out, "\\x%02x", *c); break;
                    }
                    continue;
                } else if(*c == '\\' || *c == '"') {
                    putc('\\', out);
                    putc(*c, out);
                } else {
                    putc(*c, out);
                }
            }
			fputc('"', out);
		}
	}
}

void qu_code_growval(const char *val, struct obstack *buf, const char *fmt)
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

void qu_code_print(struct qu_context *ctx, const char *template, ...) {
    va_list args;
    va_start (args, template);
    const char *c;
    for(c = template; *c; ++c) {
        if(*c == '`') {
			putc('"', ctx->out);
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
                qu_code_putval(ctx->prefix, ctx->out, fmt);
            else if(slice_equal(var, var_len, "mpref"))
                qu_code_putval(ctx->macroprefix, ctx->out, fmt);
            else {
                va_list vars;
                va_copy (vars, args);
                while(1) {
                    const char *name = va_arg(vars, char *);
                    if(!name)
                        break;
                    const char *value = va_arg(vars, char *);
                    if(slice_equal(var, var_len, name)) {
                        qu_code_putval(value, ctx->out, fmt);
                    }
                }
                va_end (vars);
            }
            c = varend;
        } else {
            putc(*c, ctx->out);
        }
    }
    va_end (args);
}

const char *qu_template_grow_va(struct qu_context *ctx,
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
