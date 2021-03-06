#include <assert.h>

#include "struct.h"
#include "context.h"
#include "guard.h"
#include "types/types.h"
#include "util/print.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"

static void qu_struct_init(struct qu_config_struct *self) {
    self->parent = NULL;
    self->path = NULL;
    self->structname = NULL;
    self->has_bitsets = 0;
    TAILQ_INIT(&self->children);
}

struct qu_config_struct *qu_struct_new_root(struct qu_context *ctx) {
    struct qu_config_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_config_struct));
    qu_struct_init(self);
    return self;
}

struct qu_config_struct *qu_struct_substruct(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name, struct qu_guard *guard)
{
    struct qu_config_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_config_struct));
    qu_struct_init(self);
    self->parent = parent;
    self->has_bitsets = parent->has_bitsets;
    if(parent->path) {
        self->path = qu_template_alloc(ctx, "${parent}.${name:c}",
            "parent", parent->path,
            "name", name,
            NULL);
    } else {
        self->path = qu_template_alloc(ctx, "${name:c}", "name", name);
    }
    struct qu_struct_member *mem = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_struct_member));
    mem->name = name;
    mem->is_struct = 1;
    mem->is_decl = 0;
    mem->p.str = self;
    mem->guard = guard;
    TAILQ_INSERT_TAIL(&parent->children, mem, lst);
    return self;
}

void qu_struct_add_option(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name,
    struct qu_option *option, struct qu_guard *guard)
{
    struct qu_struct_member *mem = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_struct_member));
    mem->name = name;
    mem->is_struct = 0;
    mem->is_decl = 0;
    mem->p.opt = option;
    mem->guard = guard;
    TAILQ_INSERT_TAIL(&parent->children, mem, lst);
    if(parent->path) {
        option->path = qu_template_alloc(ctx, "${parent}.${name:c}",
            "parent", parent->path,
            "name", name,
            NULL);
    } else {
        option->path = qu_template_alloc(ctx, "${name:c}", "name", name);
    }
}

void qu_struct_add_decl(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name,
    const char *type, struct qu_guard *guard)
{
    struct qu_struct_member *mem = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_struct_member));
    mem->name = name;
    mem->is_struct = 0;
    mem->is_decl = 1;
    mem->p.decl.type = type;
    mem->guard = guard;
    TAILQ_INSERT_TAIL(&parent->children, mem, lst);
}

void qu_struct_definition(struct qu_context *ctx, struct qu_config_struct *str)
{
    struct qu_struct_member *mem;
    if(str->has_bitsets) {
        TAILQ_FOREACH(mem, &str->children, lst) {
            if(!mem->is_struct && !mem->is_decl) {
                qu_guard_print_open(ctx, mem->guard);
                qu_code_print(ctx,
                    "unsigned int ${name:c}_set:1;\n"
                    , "name", mem->name
                    , NULL);
                qu_guard_print_close(ctx, mem->guard);
            }
        }
    }
    TAILQ_FOREACH(mem, &str->children, lst) {
        qu_guard_print_open(ctx, mem->guard);
        if(mem->is_struct) {
            if(mem->p.str->structname) {
                qu_code_print(ctx,
                    "struct ${pref}_${name:c}{\n"
                    , "name", mem->p.str->structname
                    , NULL);
            } else {
                qu_code_print(ctx,
                    "struct {\n"
                    , NULL);
            }
            qu_struct_definition(ctx, mem->p.str);
            qu_code_print(ctx,
                "} ${name:c};\n",
                "name", mem->name,
                NULL);
        } else if(mem->is_decl) {
            qu_code_print(ctx,
                "${type} ${name:c};\n"
                , "type", mem->p.decl.type
                , "name", mem->name
                , NULL);
        } else {
            mem->p.opt->vp->definition(ctx, mem->p.opt, mem->name);
        }
        qu_guard_print_close(ctx, mem->guard);
    }
}

void qu_struct_default_setter(struct qu_context *ctx,
    struct qu_config_struct *str, const char *prefix)
{
    struct qu_struct_member *mem;
    TAILQ_FOREACH(mem, &str->children, lst) {
        if(mem->is_decl)
            continue;
        qu_guard_print_open(ctx, mem->guard);
        if(mem->is_struct) {
            const char *npref = qu_template_alloc(ctx, "${prefix}${memname:c}.",
                "prefix", prefix,
                "memname", mem->name,
                NULL);
            qu_struct_default_setter(ctx, mem->p.str, npref);
        } else {
            const char *expr = qu_template_alloc(ctx, "${prefix}${memname:c}",
                "prefix", prefix,
                "memname", mem->name,
                NULL);
            mem->p.opt->vp->default_setter(ctx, mem->p.opt, expr);
        }
        qu_guard_print_close(ctx, mem->guard);
    }
}

void qu_struct_parser(struct qu_context *ctx, struct qu_config_struct *str,
    const char *prefix, int level)
{
    qu_code_print(ctx,
        "qu_ast_node *node${level:d};\n"
        "(void) node${level:d};  /* In case it is unused */\n"
        , "level:d", level+1
        , NULL);
    struct qu_struct_member *mem;
    TAILQ_FOREACH(mem, &str->children, lst) {
        if(mem->is_decl)
            continue;
        if(!mem->is_struct && mem->p.opt->cli_only)
            continue;
        qu_guard_print_open(ctx, mem->guard);
        qu_code_print(ctx,
            "if((node${nlevel:d} = qu_map_get(node${level:d}, ${name:q}))) {\n",
            "nlevel:d", level+1,
            "level:d", level,
            "name", mem->name,
            NULL);
        if(mem->is_struct) {
            const char *npref = qu_template_alloc(ctx, "${prefix}${memname:c}.",
                "prefix", prefix,
                "memname", mem->name,
                NULL);
            qu_struct_parser(ctx, mem->p.str, npref, level+1);
        } else {
            const char *expr = qu_template_alloc(ctx, "${prefix}${memname:c}",
                "prefix", prefix,
                "memname", mem->name,
                NULL);
            if(str->has_bitsets) {
                qu_code_print(ctx,
                    "${expr}_set = 1;\n"
                    , "expr", expr
                    , NULL);
            }
            mem->p.opt->vp->parser(ctx, mem->p.opt, expr, level+1);
        }
        qu_code_print(ctx,
            "}\n",
            NULL);
        qu_guard_print_close(ctx, mem->guard);
    }
}

static void qu_print_node_emitter(struct qu_context *ctx, qu_ast_node *node) {
    qu_map_member *mitem;
    qu_seq_member *sitem;

    switch(node->kind) {
    case QU_NODE_MAPPING:
        qu_code_print(ctx,
            "qu_emit_opcode(ctx, ${tag:q}, NULL, QU_EMIT_MAP_START);\n"
            , "tag", node->tag
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
            "qu_emit_opcode(ctx, ${tag:q}, NULL, QU_EMIT_SEQ_START);\n"
            , "tag", node->tag
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
            "qu_emit_scalar(ctx, ${tag:q}, NULL, QU_STYLE_PLAIN, "
                            "${data:q}, ${dlen:d});\n"
            , "tag", node->tag
            , "data", qu_node_content(node)
            , "dlen:d", strlen(qu_node_content(node))
            , NULL);
        break;
    default:
        assert(0);
    }
}

int qu_struct_needs_example(struct qu_config_struct *str) {
    struct qu_struct_member *mem;
    TAILQ_FOREACH(mem, &str->children, lst) {
        if(mem->is_decl)
            continue;
        if(mem->is_struct) {
            if(qu_struct_needs_example(mem->p.str))
                return 1;
        } else if(mem->p.opt->example) {
            return 1;
        }
    }
    return 0;
}

void qu_struct_printer(struct qu_context *ctx, struct qu_config_struct *str,
    const char *prefix, const char *tag)
{
    qu_code_print(ctx,
        "qu_emit_opcode(ctx, ${tag}, NULL, QU_EMIT_MAP_START);\n"
        , "tag", tag
        , NULL);
    struct qu_struct_member *mem;
    TAILQ_FOREACH(mem, &str->children, lst) {
        if(mem->is_decl)
            continue;
        if(mem->is_struct) {
            qu_guard_print_open(ctx, mem->guard);
            int hasex = qu_struct_needs_example(mem->p.str);
            if(!hasex) {
                qu_code_print(ctx,
                    "if(!(flags & QU_PRINT_EXAMPLE)) {\n"
                    , NULL);
            }
            qu_code_print(ctx,
                "qu_emit_scalar(ctx, NULL, NULL, 0, ${name:q}, ${namelen:d});\n"
                "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n",
                "name", mem->name,
                "namelen:d", strlen(mem->name),
                NULL);
            const char *npref = qu_template_alloc(ctx, "${prefix}${memname:c}."
                , "prefix", prefix
                , "memname", mem->name
                , NULL);
            qu_struct_printer(ctx, mem->p.str, npref, "NULL");
            if(!hasex) {
                qu_code_print(ctx, "}\n" , NULL);
            }
            qu_guard_print_close(ctx, mem->guard);
        } else {
            if(mem->p.opt->cli_only)
                continue;
            qu_guard_print_open(ctx, mem->guard);
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
            if(str->has_bitsets) {
                qu_code_print(ctx,
                    "if(${expr}_set) {\n"
                    , "expr", expr
                    , NULL);
            }
            qu_code_print(ctx,
                "qu_emit_scalar(ctx, NULL, NULL, 0, ${name:q}, ${namelen:d});\n"
                "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n",
                "name", mem->name,
                "namelen:d", strlen(mem->name),
                NULL);
            mem->p.opt->vp->printer(ctx, mem->p.opt, expr, "NULL");
            if(str->has_bitsets) {
                qu_code_print(ctx, "}\n", NULL);
            }
            qu_code_print(ctx, "}\n", NULL);
            qu_guard_print_close(ctx, mem->guard);
        }
    }
    qu_code_print(ctx,
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n"
        , NULL);
}

