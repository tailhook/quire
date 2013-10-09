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
			 /*  TODO(tailhook) quote  */
			fputs(val, out);
			fputc('"', out);
		}
	}
}

void qu_code_growval(const char *val, struct qu_context *ctx, const char *fmt)
{
	if(fmt == NULL) {
		obstack_grow(&ctx->parser.pieces, fmt, strlen(fmt));
	} else {
		if(fmt[1] == 'q') {
			obstack_1grow(&ctx->parser.pieces, '"');
			 /*  TODO(tailhook) quote  */
			obstack_grow(&ctx->parser.pieces, val, strlen(val));
			obstack_1grow(&ctx->parser.pieces, '"');
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

const char *qu_template_alloc(struct qu_context *ctx,
    const char *template, ...)
{
    va_list args;
    va_start (args, template);
    obstack_blank(&ctx->parser.pieces, 0);
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
                qu_code_growval(ctx->prefix, ctx, fmt);
            else if(slice_equal(var, var_len, "mpref"))
                qu_code_growval(ctx->macroprefix, ctx, fmt);
            else {
                va_list vars;
                va_copy (vars, args);
                while(1) {
                    const char *name = va_arg(vars, char *);
                    if(!name)
                        break;
                    const char *value = va_arg(vars, char *);
                    if(slice_equal(var, var_len, name)) {
                        qu_code_growval(value, ctx, fmt);
                    }
                }
                va_end (vars);
            }
            c = varend;
        } else {
            obstack_1grow(&ctx->parser.pieces, *c);
        }
    }
    va_end (args);
    obstack_1grow(&ctx->parser.pieces, 0);
    return obstack_finish(&ctx->parser.pieces);
}
