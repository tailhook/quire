#include "struct.h"
#include "context.h"
#include "types/types.h"
#include "util/print.h"

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

void qu_struct_parser(struct qu_context *ctx, struct qu_config_struct *str,
    const char *prefix, int level)
{
    qu_code_print(ctx,
        "qu_ast_node *node${level:d};\n",
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
        qu_code_print(ctx,
            "}\n",
            NULL);
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
            if(mem->p.opt->description) {
                qu_code_print(ctx,
                    "qu_emit_comment(ctx, 0, ${descr:q}, ${descrlen:d});\n",
                    "descr", mem->p.opt->description,
                    "descrlen:d", mem->p.opt->description,
                    NULL);
            }
            qu_code_print(ctx,
                "qu_emit_scalar(ctx, NULL, NULL, 0, ${name:q}, ${namelen:d});\n"
                "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n"
                "qu_emit_scalar(ctx, NULL, NULL, 0, ``, 0);\n",
                "name", mem->name,
                "namelen:d", strlen(mem->name),
                NULL);
        }
    }
    qu_code_print(ctx,
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n"
        , NULL);
}

