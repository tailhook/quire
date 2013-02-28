#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include "preprocessing.h"
#include "codes.h"
#include "access.h"
#include "cutil.h"

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

static int visitor(qu_context_t *ctx,
    qu_ast_node *node,
    char *expr, qu_ast_node *expr_parent) {
    qu_nodedata *data = obstack_alloc(&ctx->parsing.pieces,
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
            ctx->parsing.error_text = "Unknown type tag";
            ctx->parsing.error_token = node->tag;
            ctx->parsing.error_kind = YAML_CONTENT_ERROR;
            longjmp(ctx->parsing.errjmp, 1);
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
                ctx->parsing.error_text = "Element type not specified";
                ctx->parsing.error_token = node->tag;  // better place?
                ctx->parsing.error_kind = YAML_CONTENT_ERROR;
                longjmp(ctx->parsing.errjmp, 1);
            }
            visitor(ctx, elem, ".value", expr_parent);
            qu_nodedata *edata = elem->userdata;
            char *tname = typealias[edata->type];
            if(tname == NULL) {
                switch(edata->kind) {
                case QU_MEMBER_CUSTOM:
                    tname = edata->data.custom.typename;
                    break;
                default:
                    ctx->parsing.error_text = "Nested arrays and maps "
                                              "are not supported";
                    ctx->parsing.error_token = node->tag;  // better place?
                    ctx->parsing.error_kind = YAML_CONTENT_ERROR;
                    longjmp(ctx->parsing.errjmp, 1);
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
                ctx->parsing.error_text = "Key or value type not specified";
                ctx->parsing.error_token = node->tag;  // better place?
                ctx->parsing.error_kind = YAML_CONTENT_ERROR;
                longjmp(ctx->parsing.errjmp, 1);
            }
            visitor(ctx, kelem, ".key", expr_parent);
            visitor(ctx, velem, ".value", expr_parent);
            qu_nodedata *kedata = kelem->userdata;
            char *ktname = typealias[kedata->type];
            if(ktname == NULL) {
                switch(kedata->kind) {
                case QU_MEMBER_CUSTOM:
                    ktname = kedata->data.custom.typename;
                    break;
                default:
                    ctx->parsing.error_text = "Nested arrays and maps "
                                              "are not supported";
                    ctx->parsing.error_token = node->tag;  // better place?
                    ctx->parsing.error_kind = YAML_CONTENT_ERROR;
                    longjmp(ctx->parsing.errjmp, 1);
                }
            }
            qu_nodedata *vedata = velem->userdata;
            char *vtname = typealias[vedata->type];
            if(vtname == NULL) {
                switch(vedata->kind) {
                case QU_MEMBER_CUSTOM:
                    vtname = vedata->data.custom.typename;
                    break;
                default:
                    ctx->parsing.error_text = "Nested arrays and maps "
                                              "are not supported";
                    ctx->parsing.error_token = node->tag;  // better place?
                    ctx->parsing.error_kind = YAML_CONTENT_ERROR;
                    longjmp(ctx->parsing.errjmp, 1);
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
                    ctx->parsing.error_text = "Type not specified";
                    ctx->parsing.error_token = node->tag;  // better place?
                    ctx->parsing.error_kind = YAML_CONTENT_ERROR;
                    longjmp(ctx->parsing.errjmp, 1);
                }
            }
            char *typename = qu_node_content(mnode);
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
            char *mname = qu_node_content(item->key);
            if(!*mname || *mname == '_')
                continue;
            obstack_blank(&ctx->parsing.pieces, 0);
            obstack_grow(&ctx->parsing.pieces, expr, strlen(expr));
            obstack_grow(&ctx->parsing.pieces, ".", 1);
            qu_append_c_name(&ctx->parsing.pieces, mname);
            char *nexpr = (char *)obstack_finish(&ctx->parsing.pieces);
            visitor(ctx, item->value, nexpr, expr_parent);
        }
    }
    return 0;
}

int qu_config_preprocess(qu_context_t *ctx) {
    int rc;
    ctx->parsing.has_jmp = 1;
    if(!(rc = setjmp(ctx->parsing.errjmp))) {

        _qu_parse_metadata(ctx);

        ctx->prefix = ctx->options.prefix;
        int len = strlen(ctx->prefix);
        ctx->macroprefix = obstack_copy0(&ctx->parsing.pieces,
            ctx->prefix, len);
        for(int i = 0; i < len; ++i)
            ctx->macroprefix[i] = toupper(ctx->macroprefix[i]);

        TAILQ_INIT(&ctx->cli_options);
        visitor(ctx, ctx->parsing.document, "", NULL);
        qu_ast_node *types = qu_map_get(ctx->parsing.document, "__types__");
        if(types) {
            if(types->kind != QU_NODE_MAPPING) {
                ctx->parsing.error_text = "__types__ must be mapping";
                ctx->parsing.error_token = types->start_token;
                ctx->parsing.error_kind = YAML_CONTENT_ERROR;
                longjmp(ctx->parsing.errjmp, 1);
            }
            qu_map_member *item;
            TAILQ_FOREACH(item, &types->val.map_index.items, lst) {
                visitor(ctx, item->value, "", NULL);
            }
        }

    } else {
        ctx->parsing.has_jmp = 0;
        return rc;
    }
    ctx->parsing.has_jmp = 0;
    return 0;
}
