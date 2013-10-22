#include <errno.h>
#include <string.h>

#include "error.h"
#include "quire_int.h"

int qu_print_error(qu_parse_context *ctx, FILE *stream) {
    int rc;
    int keep_errno = errno;
    if(ctx->error_token) {
        rc = fprintf(stream, "Error parsing file %s:%d: %s\n",
            ctx->filename, ctx->error_token->start_line,
            ctx->error_text);
        if(rc)
            return -errno;
    } else if(ctx->error_text) {
        rc = fprintf(stream, "Error parsing file %s: %s\n",
            ctx->filename, ctx->error_text);
        if(rc)
            return -errno;
    } else {
        rc = fprintf(stream, "Error parsing file %s: %s\n",
            ctx->filename, strerror(keep_errno));
        if(rc)
            return -errno;
    }
    return 0;
}

void qu_report_error(qu_parse_context *ctx, qu_ast_node *node,
    const char *text)
{
    if(node) {
        LONGJUMP_WITH_CONTENT_ERROR(ctx, node->start_token, text);
    } else {
        LONGJUMP_WITH_CONTENT_ERROR(ctx, NULL, text);
    }
}
