#include <assert.h>

#include "struct.h"
#include "context.h"
#include "types/types.h"
#include "util/print.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"

static void qu_struct_init(struct qu_config_struct *self) {
    self->parent = NULL;
    TAILQ_INIT(&self->children);
}

struct qu_config_struct *qu_struct_new_root(struct qu_context *ctx) {
    struct qu_config_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_config_struct));
    qu_struct_init(self);
    return self;
}

struct qu_config_struct *qu_struct_substruct(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name)
{
    struct qu_config_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_config_struct));
    qu_struct_init(self);
    self->parent = parent;
    if(parent->path) {
        self->path = qu_template_alloc(ctx, "${parent}.${name}",
            "parent", parent->path,
            "name", name,
            NULL);
    } else {
        self->path = name;
    }
    struct qu_struct_member *mem = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_struct_member));
    mem->name = name;
    mem->is_struct = 1;
    mem->p.str = self;
    TAILQ_INSERT_TAIL(&parent->children, mem, lst);
    return self;
}

void qu_struct_add_option(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name, struct qu_option *option)
{
    struct qu_struct_member *mem = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_struct_member));
    mem->name = name;
    mem->is_struct = 0;
    mem->p.opt = option;
    TAILQ_INSERT_TAIL(&parent->children, mem, lst);
    if(parent->path) {
        option->path = qu_template_alloc(ctx, "${parent}.${name}",
            "parent", parent->path,
            "name", name,
            NULL);
    } else {
        option->path = name;
    }
}

void qu_struct_definition(struct qu_context *ctx, struct qu_config_struct *str)
{
    struct qu_struct_member *mem;
    TAILQ_FOREACH(mem, &str->children, lst) {
        if(mem->is_struct) {
        } else {
            mem->p.opt->vp->definition(ctx, mem->p.opt, mem->name);
        }
    }
}

void qu_struct_parser(struct qu_context *ctx, struct qu_config_struct *str,
    const char *prefix, int level)
{
    qu_code_print(ctx,
        "qu_ast_node *node${level:d};\n",
        "level:d", level+1,
        NULL);
    const char *nname = qu_template_alloc(ctx, "node${level:d}",
        "level:d", level+1,
        NULL);
    struct qu_struct_member *mem;
    TAILQ_FOREACH(mem, &str->children, lst) {
        qu_code_print(ctx,
            "if((node${nlevel:d} = qu_map_get(node${level:d}, ${name:q}))) {\n",
            "nlevel:d", level+1,
            "level:d", level,
            "name", mem->name,
            NULL);
        const char *expr = qu_template_alloc(ctx, "${prefix}${memname:c}",
            "prefix", prefix,
            "memname", mem->name,
            NULL);
        if(mem->is_struct) {
        } else {
            mem->p.opt->vp->parser(ctx, mem->p.opt, expr, nname);
        }
        qu_code_print(ctx,
            "}\n",
            NULL);
    }
    obstack_free(&ctx->parser.pieces, nname);
}

static void qu_print_node_emitter(struct qu_context *ctx, qu_ast_node *node) {
    qu_map_member *mitem;
    qu_seq_member *sitem;

    switch(node->kind) {
    case QU_NODE_MAPPING:
        qu_code_print(ctx,
            "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_START);\n"
            , NULL);
        TAILQ_FOREACH(mitem, &node->val.map_index.items, lst) {
            qu_print_node_emitter(ctx, mitem->key);
            qu_code_print(ctx,
                "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n"
                , NULL);
            qu_print_node_emitter(ctx, mitem->value);
        }
        qu_code_print(ctx,
            "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n"
            , NULL);
        break;
    case QU_NODE_SEQUENCE:
        qu_code_print(ctx,
            "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_START);\n"
            , NULL);
        TAILQ_FOREACH(sitem, &node->val.seq_index.items, lst) {
            qu_code_print(ctx,
                "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_ITEM);\n"
                , NULL);
            qu_print_node_emitter(ctx, sitem->value);
        }
        qu_code_print(ctx,
            "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_END);\n"
            , NULL);
        break;
    case QU_NODE_SCALAR:
        qu_code_print(ctx,
            "qu_emit_scalar(ctx, NULL, NULL, QU_STYLE_PLAIN, "
                            "${data:q}, ${dlen:d});\n",
            "data", qu_node_content(node),
            "dlen:d", strlen(qu_node_content(node)),
            NULL);
        break;
    default:
        assert(0);
    }
}

void qu_struct_printer(struct qu_context *ctx, struct qu_config_struct *str,
    const char *prefix)
{
    qu_code_print(ctx,
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_START);\n"
        , NULL);
    struct qu_struct_member *mem;
    TAILQ_FOREACH(mem, &str->children, lst) {
        if(mem->is_struct) {
        } else {
            const char *expr = qu_template_alloc(ctx, "${prefix}${memname:c}",
                "prefix", prefix,
                "memname", mem->name,
                NULL);
            if(mem->p.opt->example || !mem->p.opt->has_default) {
                qu_code_print(ctx,
                    "if(flags & QU_PRINT_EXAMPLE) {\n"
                    , NULL);
                if(mem->p.opt->description) {
                    qu_code_print(ctx,
                        "if(flags & QU_PRINT_COMMENTS)\n"
                        "    qu_emit_comment(ctx, 0, ${descr:q}, ${descrlen:d});\n",
                        "descr", mem->p.opt->description,
                        "descrlen:d", strlen(mem->p.opt->description),
                        NULL);
                }
                qu_code_print(ctx,
                    "qu_emit_scalar(ctx, NULL, NULL, 0, ${name:q}, ${namelen:d});\n"
                    "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n",
                    "name", mem->name,
                    "namelen:d", strlen(mem->name),
                    NULL);
                if(mem->p.opt->example) {
                    qu_print_node_emitter(ctx, mem->p.opt->example);
                } else {
                    qu_code_print(ctx,
                        "qu_emit_scalar(ctx, NULL, NULL, 0, ``, 0);\n"
                        "qu_emit_comment(ctx, QU_COMMENT_SAME_LINE, "
                                        "`required`, 8);\n"
                        , NULL);
                }
                qu_code_print(ctx, "}\n", NULL);
            }
            qu_code_print(ctx,
                "if(!(flags & QU_PRINT_EXAMPLE)) {\n"
                , NULL);
            if(mem->p.opt->description) {
                qu_code_print(ctx,
                    "if(flags & QU_PRINT_COMMENTS)\n"
                    "    qu_emit_comment(ctx, 0, ${descr:q}, ${descrlen:d});\n",
                    "descr", mem->p.opt->description,
                    "descrlen:d", strlen(mem->p.opt->description),
                    NULL);
            }
            qu_code_print(ctx,
                "qu_emit_scalar(ctx, NULL, NULL, 0, ${name:q}, ${namelen:d});\n"
                "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n",
                "name", mem->name,
                "namelen:d", strlen(mem->name),
                NULL);
            mem->p.opt->vp->printer(ctx, mem->p.opt, expr);
            qu_code_print(ctx, "}\n", NULL);
        }
    }
    qu_code_print(ctx,
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n"
        , NULL);
}

