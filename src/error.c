#include <errno.h>
#include <string.h>

#include "yparser.h"
#include "error.h"

int qu_print_error(qu_parse_context *ctx, FILE *stream) {
    int rc;
    char *base = strrchr(ctx->filename, '/');
    if(base)
        base += 1;
    else
        base = ctx->filename;
    rc = fprintf(stream, "Error parsing file %s:%d: %s\n",
        base, ctx->error_token->start_line,
        ctx->error_text);
    if(rc)
        return -errno;
    return 0;
}
