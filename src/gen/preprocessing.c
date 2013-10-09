#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include "preprocessing.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"
#include "util/name.h"
#include "context.h"
#include "metadata.h"

static char *typealias[] = {
    [QU_TYP_INT] = "long",
    [QU_TYP_FLOAT] = "double",
    [QU_TYP_FILE] = "file",
    [QU_TYP_DIR] = "dir",
    [QU_TYP_STRING] = "string",
    [QU_TYP_BOOL] = "bool",
    [QU_TYP_CUSTOM] = NULL,
    [QU_TYP_ARRAY] = NULL,
    [QU_TYP_MAP] = NULL,
    };

static int visitor(struct qu_context *ctx,
    qu_ast_node *node,
    const char *expr, qu_ast_node *expr_parent) {
    qu_nodedata *data = obstack_alloc(&ctx->parser.pieces,
                                      sizeof(qu_nodedata));
    memset(data, 0, sizeof(qu_nodedata));
    if(!data)
        return -ENOMEM;
    data->expression = expr;
    data->expr_parent = expr_parent;
    data->node = node;
    node->userdata = data;
    if(node->tag) {
        int tlen = node->tag->bytelen;
        char *tdata = (char *)node->tag->data;
        if(tlen == 4 && !strncmp(tdata, "!Int", 4))
            data->type = QU_TYP_INT;
        else if(tlen == 6 && !strncmp(tdata, "!Float", 6))
            data->type = QU_TYP_FLOAT;
        else if(tlen == 5 && !strncmp(tdata, "!File", 5))
            data->type = QU_TYP_FILE;
        else if(tlen == 4 && !strncmp(tdata, "!Dir", 4))
            data->type = QU_TYP_DIR;
        else if(tlen == 7 && !strncmp(tdata, "!String", 7))
            data->type = QU_TYP_STRING;
        else if(tlen == 5 && !strncmp(tdata, "!Bool", 5))
            data->type = QU_TYP_BOOL;
        else if(tlen == 7 && !strncmp(tdata, "!Struct", 7))
            data->type = QU_TYP_CUSTOM;
        else if(tlen == 6 && !strncmp(tdata, "!Array", 6))
            data->type = QU_TYP_ARRAY;
        else if(tlen == 8 && !strncmp(tdata, "!Mapping", 8))
            data->type = QU_TYP_MAP;
        else {
            LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                node->tag, "Unknown type tag");
        }
        switch(data->type) {
        case QU_TYP_INT:
        case QU_TYP_FLOAT:
        case QU_TYP_FILE:
        case QU_TYP_DIR:
        case QU_TYP_STRING:
        case QU_TYP_BOOL:
            data->kind = QU_MEMBER_SCALAR;
            break;
        case QU_TYP_ARRAY:
            data->kind = QU_MEMBER_ARRAY;
            qu_ast_node *elem = qu_map_get(node, "element");
            if(!elem) {
                LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                    node->tag, "Element type not specified");
            }
            visitor(ctx, elem, ".value", expr_parent);
            qu_nodedata *edata = elem->userdata;
            const char *tname = typealias[edata->type];
            if(tname == NULL) {
                switch(edata->kind) {
                case QU_MEMBER_CUSTOM:
                    tname = edata->data.custom.typename;
                    break;
                default:
                    LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                        node->tag, "Nested arrays and maps "
                                   "are not supported");
                }
            }
            data->data.array.membername = tname;
            qu_nodedata *oldarr = NULL;
            TAILQ_FOREACH(oldarr, &ctx->arrays, data.array.lst) {
                if(!strcmp(oldarr->data.array.membername, tname)) {
                    break;
                }
            }
            if(!oldarr) {
                TAILQ_INSERT_HEAD(&ctx->arrays, data, data.array.lst);
            }
            break;
        case QU_TYP_MAP:
            data->kind = QU_MEMBER_MAP;
            qu_ast_node *kelem = qu_map_get(node, "key-element");
            qu_ast_node *velem = qu_map_get(node, "value-element");
            if(!kelem || !velem) {
                LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                    node->tag, "Key or value type not specified");
            }
            visitor(ctx, kelem, ".key", expr_parent);
            visitor(ctx, velem, ".value", expr_parent);
            qu_nodedata *kedata = kelem->userdata;
            const char *ktname = typealias[kedata->type];
            if(ktname == NULL) {
                switch(kedata->kind) {
                case QU_MEMBER_CUSTOM:
                    ktname = kedata->data.custom.typename;
                    break;
                default:
                    LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                        node->tag, "Nested arrays and maps "
                                   "are not supported");
                }
            }
            qu_nodedata *vedata = velem->userdata;
            const char *vtname = typealias[vedata->type];
            if(vtname == NULL) {
                switch(vedata->kind) {
                case QU_MEMBER_CUSTOM:
                    vtname = vedata->data.custom.typename;
                    break;
                default:
                    LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                        node->tag, "Nested arrays and maps "
                                   "are not supported");
                }
            }
            assert(ktname && vtname);
            data->data.mapping.keyname = ktname;
            data->data.mapping.valuename = vtname;
            qu_nodedata *oldmapping = NULL;
            TAILQ_FOREACH(oldmapping, &ctx->mappings, data.mapping.lst) {
                if(!strcmp(oldmapping->data.mapping.keyname, ktname) ||
                   !strcmp(oldmapping->data.mapping.valuename, ktname)) {
                    break;
                }
            }
            if(!oldmapping) {
                TAILQ_INSERT_HEAD(&ctx->mappings, data, data.mapping.lst);
            }
            break;
        case QU_TYP_CUSTOM:
            data->kind = QU_MEMBER_CUSTOM;
            qu_ast_node *mnode = node;
            if(node->kind == QU_NODE_MAPPING) {
                mnode = qu_map_get(node, "=");
                if(!mnode) mnode = qu_map_get(node, "type");
                if(!mnode) {
                    LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                        node->tag, "Type not specified");
                }
            }
            const char *typename = qu_node_content(mnode);
            data->data.custom.typename = typename;
            break;
        }
        if(node->kind == QU_NODE_MAPPING) {
            if(qu_map_get(node, "command-line")
                || qu_map_get(node, "command-line-incr")
                || qu_map_get(node, "command-line-decr")
                || qu_map_get(node, "command-line-enable")
                || qu_map_get(node, "command-line-disable")) {
                data->cli_name = strrchr(expr, '.')+1;
                TAILQ_INSERT_TAIL(&ctx->cli_options, data, cli_lst);
            }
        }
    } else if(node->kind == QU_NODE_MAPPING) {
        data->kind = QU_MEMBER_STRUCT;
        qu_map_member *item;
        TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
            const char *mname = qu_node_content(item->key);
            if(!*mname || *mname == '_')
                continue;
            obstack_blank(&ctx->parser.pieces, 0);
            obstack_grow(&ctx->parser.pieces, expr, strlen(expr));
            obstack_grow(&ctx->parser.pieces, ".", 1);
            qu_append_c_name(&ctx->parser.pieces, mname);
            const char *nexpr = (char *)obstack_finish(&ctx->parser.pieces);
            visitor(ctx, item->value, nexpr, expr_parent);
        }
        qu_ast_node *vnode = qu_map_get(node, "__value__");
        if(vnode && vnode->tag && (vnode->tag->bytelen != 8
            || strncmp((char *)vnode->tag->data, "!Convert", 8))) {
            visitor(ctx, vnode, ".value", expr_parent);
        }
    }
    return 0;
}

void qu_config_preprocess(struct qu_context *ctx) {
    int rc;
    assert(ctx->parser.errjmp);

    qu_parse_metadata(ctx);

    TAILQ_INIT(&ctx->cli_options);
    visitor(ctx, ctx->parser.document, "", NULL);
    qu_ast_node *types = qu_map_get(ctx->parser.document, "__types__");
    if(types) {
        if(types->kind != QU_NODE_MAPPING) {
            LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                types->tag, "__types__ must be mapping");
        }
        qu_map_member *typ;
        TAILQ_FOREACH(typ, &types->val.map_index.items, lst) {
            qu_ast_node *tags = qu_map_get(typ->value, "__tags__");
            if(tags && !tags->userdata) {
                qu_map_member *item;
                TAILQ_FOREACH(item, &tags->val.map_index.items, lst) {
                    if(qu_node_content(item->key)[0] == '_')
                        continue;
                    obstack_blank(&ctx->parser.pieces, 0);
                    obstack_grow(&ctx->parser.pieces,
                        ctx->macroprefix, strlen(ctx->macroprefix));
                    qu_append_c_name(&ctx->parser.pieces,
                        qu_node_content(item->key));
                    item->value->userdata = obstack_finish(
                        &ctx->parser.pieces);
                }
                tags->userdata = qu_node_content(typ->key);
            }
            visitor(ctx, typ->value, "", NULL);
        }
    }
}
