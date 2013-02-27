#include <errno.h>
#include <string.h>

#include "yparser.h"
#include "error.h"

int qu_print_error(qu_parse_context *ctx, FILE *stream) {
    int rc;
	int keep_errno = errno;
    char *base = strrchr(ctx->filename, '/');
    if(base)
        base += 1;
    else
        base = ctx->filename;
	if(ctx->error_token) {
		rc = fprintf(stream, "Error parsing file %s:%d: %s\n",
			base, ctx->error_token->start_line,
			ctx->error_text);
		if(rc)
			return -errno;
	} else {
		rc = fprintf(stream, "Error parsing file %s: %s\n",
			base, strerror(keep_errno));
		if(rc)
			return -errno;
	}
    return 0;
}
