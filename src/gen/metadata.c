#include <stdio.h>

#include "metadata.h"
#include "../yaml/access.h"
#include "context.h"
#include "../yaml/codes.h"

void qu_parse_metadata(struct qu_context *bigctx) {
    qu_parse_context *ctx = &bigctx->parser;
    qu_ast_node *root = ctx->document;
    qu_metadata_t *meta = &bigctx->meta;
    meta->program_name = NULL;
    meta->default_config = NULL;
    meta->description = NULL;
    meta->has_arguments = 0;
    meta->mixed_arguments = 0;
    qu_ast_node *mnode = qu_map_get(root, "__meta__");
    if(!mnode)
        return;

    qu_ast_node *tnode;
    const char *tdata;

    tnode = qu_map_get(mnode, "program-name");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            LONGJUMP_WITH_CONTENT_ERROR(ctx, tnode->start_token,
                "__meta__.program-name must be scalar");
        }
        meta->program_name = tdata;
    }

    tnode = qu_map_get(mnode, "default-config");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            LONGJUMP_WITH_CONTENT_ERROR(ctx, tnode->start_token,
                "__meta__.default-config must be scalar");
        }
        meta->default_config = tdata;
    }

    tnode = qu_map_get(mnode, "description");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            LONGJUMP_WITH_CONTENT_ERROR(ctx, tnode->start_token,
                "__meta__.description must be scalar");
        }
        meta->description = tdata;
    }

}
