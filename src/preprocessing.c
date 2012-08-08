#include <ctype.h>

#include "preprocessing.h"


int qu_config_preprocess(qu_context_t *ctx) {

    int rc = qu_parse_metadata(ctx->parsing.document, &ctx->meta);
    if(rc < 0)
        return rc;

    ctx->prefix = ctx->options.prefix;
    int len = strlen(ctx->prefix);
    ctx->macroprefix = obstack_copy0(&ctx->parsing.pieces, ctx->prefix, len);
    for(int i = 0; i < len; ++i)
        ctx->macroprefix[i] = toupper(ctx->macroprefix[i]);

    return 0;
}
