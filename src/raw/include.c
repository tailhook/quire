#include "include.h"
#include "../yaml/access.h"

qu_ast_node *qu_raw_include(qu_parse_context *ctx, qu_ast_node *src) {
    char *included_file = qu_node_content(src);
    char *fn = qu_join_filenames(ctx, src->tag->filename, included_file);
    return qu_file_newparse(ctx, fn);
}
