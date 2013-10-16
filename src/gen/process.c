#include <assert.h>
#include <ctype.h>

#include "metadata.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"
#include "types/types.h"
#include "util/print.h"
#include "special/special.h"
#include "struct.h"
#include "context.h"

static void qu_parse_common(struct qu_context *ctx, struct qu_option *opt,
    qu_ast_node *node) {
    qu_ast_node *tmp;
    if((tmp = qu_map_get(node, "description")))
        opt->description = qu_node_content(tmp);
    if((tmp = qu_map_get(node, "example")))
        opt->example = tmp;
    opt->has_default = 0;
}

struct qu_option *qu_parse_option(struct qu_context *ctx, qu_ast_node *node,
    const char *name, struct qu_config_struct *parent)
{
    struct qu_option *opt = qu_option_resolve(ctx,
        (char *)node->tag->data, node->tag->bytelen);
    if(!opt->vp) {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
            node->tag ? node->tag : node->start_token,
            "Unknown object type");
    }
    qu_parse_common(ctx, opt, node);
    if(parent && parent->path) {
        opt->path = qu_template_alloc(ctx, "${parent}.${name}",
            "parent", parent->path,
            "name", name,
            NULL);
    } else {
        opt->path = name;
    }
    opt->vp->parse(ctx, opt, node);
    return opt;
}

void qu_visit_struct_children(struct qu_context *ctx,
    qu_ast_node *node, struct qu_config_struct *str)
{
    assert (node->kind == QU_NODE_MAPPING);
    qu_map_member *item;
    TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
        const char *mname = qu_node_content(item->key);
        if(!*mname || *mname == '_') {
            qu_special_parse(ctx, mname, item->value);
            continue;
        }
        if(!item->value->tag) {
            if(item->value->kind == QU_NODE_MAPPING) {
                struct qu_config_struct *child;
                child = qu_struct_substruct(ctx, str, mname);
                qu_visit_struct_children(ctx, item->value, child);
            } else {
                LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                    item->value->start_token,
                    "Untagged straw scalar");
            }
        } else {
            struct qu_option *opt = qu_parse_option(ctx,
                item->value, mname, str);
            qu_struct_add_option(ctx, str, mname, opt);
            if(item->value->kind == QU_NODE_MAPPING) {
                qu_cli_parse(ctx, opt, item->value);
            }
        }
    }
}

static void qu_config_set_prefix(struct qu_context *ctx) {
    ctx->prefix = ctx->options.prefix;
    int len = strlen(ctx->prefix);
    char * macroprefix = obstack_copy0(&ctx->parser.pieces,
        ctx->prefix, len);
    for(int i = 0; i < len; ++i)
        macroprefix[i] = toupper(macroprefix[i]);
    ctx->macroprefix = macroprefix;
}

void qu_config_preprocess(struct qu_context *ctx) {
    qu_parse_metadata(ctx);
    assert (ctx->meta.program_name);
    qu_config_set_prefix(ctx);
    qu_cli_add_quire(ctx);
    ctx->root = qu_struct_new_root(ctx);
    qu_visit_struct_children(ctx, ctx->parser.document, ctx->root);
}
