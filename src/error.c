#include <errno.h>

#include "yparser.h"
#include "error.h"

int qu_has_error(qu_parse_context *ctx) {
    return !!ctx->error_kind;
}
int qu_print_error(qu_parse_context *ctx, FILE *stream) {
    int rc;
    rc = fprintf(stream, "Error parsing file %s:%d: %s\n",
        ctx->filename, ctx->error_token->start_line,
        ctx->error_text);
    if(rc)
        return -errno;
    return 0;
}
