#include <stdarg.h>
#include <assert.h>

#include "print.h"
#include "../context.h"

#define slice_equal(start, len, val) \
    ((len) == strlen((val)) && !strncmp((val), (start), (len)))

void qu_code_print(struct qu_context *ctx, const char *template, ...) {
    va_list args;
    va_start (args, template);
    const char *c;
    for(c = template; *c; ++c) {
        if(*c == '`') {
            const char *varend = strchr(c+1, '`');
            const char *var = c+1;
            int var_len = varend - var;
            assert(varend);
            if(slice_equal(var, var_len, "pref"))
                fputs(ctx->prefix, ctx->out);
            else if(slice_equal(var, var_len, "mpref"))
                fputs(ctx->macroprefix, ctx->out);
            else {
                va_list vars;
                va_copy (vars, args);
                while(1) {
                    const char *name = va_arg(vars, char *);
                    if(!name)
                        break;
                    const char *value = va_arg(vars, char *);
                    if(slice_equal(var, var_len, name)) {
                        fputs(value, ctx->out);
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
            const char *varend = strchr(c+1, '`');
            const char *var = c+1;
            int var_len = varend - var;
            assert(varend);
            if(slice_equal(var, var_len, "pref"))
                fputs(ctx->prefix, ctx->out);
            else if(slice_equal(var, var_len, "mpref"))
                fputs(ctx->macroprefix, ctx->out);
            else {
                va_list vars;
                va_copy (vars, args);
                while(1) {
                    const char *name = va_arg(vars, char *);
                    if(!name)
                        break;
                    const char *value = va_arg(vars, char *);
                    if(slice_equal(var, var_len, name)) {
                        obstack_grow(&ctx->parser.pieces, value, strlen(value));
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
