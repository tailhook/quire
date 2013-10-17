#include "include.h"
#include "../yaml/access.h"

qu_ast_node *qu_raw_include(qu_parse_context *ctx, qu_ast_node *src) {
    const char *included_file = qu_node_content(src);
    const char *fn = qu_join_filenames(ctx,
        src->tag_token->filename, included_file);
    return qu_file_newparse(ctx, fn);
}
