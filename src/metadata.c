#include <stdio.h>

#include "metadata.h"
#include "access.h"
#include "context.h"
#include "codes.h"

void _qu_parse_metadata(qu_context_t *bigctx) {
    qu_parse_context *ctx = &bigctx->parsing;
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
    char *tdata;

    tnode = qu_map_get(mnode, "program-name");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            ctx->error_token = tnode->start_token;
            ctx->error_kind = YAML_CONTENT_ERROR;
            ctx->error_text = "__meta__.program-name must be scalar";
            longjmp(ctx->errjmp, 1);
        }
        meta->program_name = tdata;
    }

    tnode = qu_map_get(mnode, "default-config");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            ctx->error_token = tnode->start_token;
            ctx->error_kind = YAML_CONTENT_ERROR;
            ctx->error_text = "__meta__.default-config must be scalar";
            longjmp(ctx->errjmp, 1);
        }
        meta->default_config = tdata;
    }

    tnode = qu_map_get(mnode, "description");
    if(tnode) {
        tdata = qu_node_content(tnode);
        if(!tdata) {
            ctx->error_token = tnode->start_token;
            ctx->error_kind = YAML_CONTENT_ERROR;
            ctx->error_text = "__meta__.description must be scalar";
            longjmp(ctx->errjmp, 1);
        }
        meta->description = tdata;
    }

    tnode = qu_map_get(mnode, "has-arguments");
    if(tnode) {
        int value;
        if(qu_get_boolean(tnode, &value) == -1) {
            ctx->error_token = tnode->start_token;
            ctx->error_kind = YAML_CONTENT_ERROR;
            ctx->error_text = "__meta__.has-arguments must be boolean";
            longjmp(ctx->errjmp, 1);
        }
        meta->has_arguments = value;
    }

    tnode = qu_map_get(mnode, "mixed-arguments");
    if(tnode) {
        int value;
        if(qu_get_boolean(tnode, &value) == -1) {
            ctx->error_token = tnode->start_token;
            ctx->error_kind = YAML_CONTENT_ERROR;
            ctx->error_text = "__meta__.mixed-arguments must be boolean";
            longjmp(ctx->errjmp, 1);
        }
        if(value && !meta->has_arguments) {
            ctx->error_token = tnode->start_token;
            ctx->error_kind = YAML_CONTENT_ERROR;
            ctx->error_text = ("__meta__.mixed-arguments without has-arguments"
                " is useless");
            longjmp(ctx->errjmp, 1);
        }
        meta->mixed_arguments = value;
    }

}
