#include "include.h"
#include "../yaml/access.h"

qu_ast_node *qu_raw_include(struct qu_parser *ctx, qu_ast_node *src) {
    const char *included_file = qu_node_content(src);
    const char *fn = qu_join_filenames(ctx,
        src->tag_token->filename, included_file);
    qu_ast_node *node = qu_file_newparse(ctx, fn);
    if(!node)
        return qu_new_text_node(ctx, NULL);
    return node;
}
