#include <errno.h>
#include <string.h>

#include "error.h"
#include "quire_int.h"

void qu_print_error(qu_parse_context *ctx, FILE *stream) {
    switch(ctx->error_kind) {
    case QU_ERR_VALUE:
        fprintf(stream, "Value error at \"%s\":%d: %s\n",
            ctx->err_ptr.token->filename, ctx->err_ptr.token->start_line,
            ctx->error_text);
        break;
    case QU_ERR_SYNTAX:
        fprintf(stream, "Syntax error at \"%s\":%d: %s\n",
            ctx->err_ptr.token->filename, ctx->err_ptr.token->start_line,
            ctx->error_text);
        break;
    case QU_ERR_ERRNO:
        fprintf(stream, "System error, file \"%s\": %s\n",
            ctx->filename, strerror(ctx->err_ptr.errnum));
        break;
    case QU_ERR_SYSTEM:
        fprintf(stream, "System error at \"%s\":%d: %s\n",
            ctx->err_ptr.token->filename, ctx->err_ptr.token->start_line,
            ctx->error_text);
        break;
    case QU_ERR_CMDLINE:
        if(ctx->err_ptr.cli_option[0] == '-') {  /*  Long option  */
            fprintf(stream, "Option \"%s\": %s\n",
                ctx->err_ptr.cli_option, ctx->error_text);
        } else {
            fprintf(stream, "Option \"-%c\": %s\n",
                ctx->err_ptr.cli_option[0], ctx->error_text);
        }
        break;
    }
}

void qu_report_error(qu_parse_context *ctx, qu_ast_node *node,
    const char *text)
{
    ctx->error_kind = QU_ERR_VALUE;
    ctx->error_text = text;
    ctx->err_ptr.token = (node)->tag_token ? \
        (node)->tag_token : (node)->start_token;
    if(ctx->errjmp) {
        longjmp(*(ctx)->errjmp, QU_YAML_ERROR);
    } else {
        qu_print_error(ctx, stderr);
        abort();
    }
}

void qu_cmdline_error(qu_parse_context *ctx, const char *opt, const char *text)
{
    ctx->error_kind = QU_ERR_CMDLINE;
    ctx->error_text = text;
    ctx->err_ptr.cli_option = opt;
    if(ctx->errjmp) {
        longjmp(*(ctx)->errjmp, QU_YAML_ERROR);
    } else {
        qu_print_error(ctx, stderr);
        abort();
    }
}
