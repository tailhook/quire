#include <stdio.h>

#include "metadata.h"
#include "../yaml/access.h"
#include "context.h"
#include "../yaml/codes.h"

void qu_parse_metadata(struct qu_context *bigctx) {
    struct qu_parser *ctx = &bigctx->parser;
    qu_ast_node *root = ctx->document;
    qu_metadata_t *meta = &bigctx->meta;
    meta->program_name = "unknown_program";
    meta->default_config = "/etc/unknown_program.yaml";
    meta->description = "";
    qu_ast_node *mnode = qu_map_get(root, "__meta__");
    if(!mnode)
        return;

    qu_ast_node *tnode;
    const char *tdata;

    tnode = qu_map_get(mnode, "program-name");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            qu_err_node_error(ctx->err, tnode,
                "__meta__.program-name must be scalar");
        } else {
            meta->program_name = tdata;
        }
    }

    tnode = qu_map_get(mnode, "default-config");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            qu_err_node_error(ctx->err, tnode,
                "__meta__.default-config must be scalar");
        } else {
            meta->default_config = tdata;
        }
    }

    tnode = qu_map_get(mnode, "description");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            qu_err_node_error(ctx->err, tnode,
                "__meta__.description must be scalar");
        } else {
            meta->description = tdata;
        }
    }

}
