#include "struct.h"
#include "classes.h"
#include "../struct.h"
#include "../process.h"
#include "../context.h"
#include "../util/print.h"
#include "../util/print.h"

struct qu_class_struct {
    struct qu_config_struct *body;
};

static void qu_struct_init(struct qu_context *, struct qu_class *,
    qu_ast_node *node);
static void qu_struct_func_declare(struct qu_context *, struct qu_class *);
static void qu_struct_func_body(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_struct = {
    /* init */ qu_struct_init,
    /* func_declare */ qu_struct_func_declare,
    /* func_body */ qu_struct_func_body
    };

static void qu_struct_print(struct qu_context *ctx, struct qu_class *cls) {
    struct qu_class_struct *self = cls->classdata;
    qu_code_print(ctx,
        "struct ${pref}_${typename} {\n"
        , "typename", cls->name
        , NULL);
    qu_struct_definition(ctx, self->body);
    qu_code_print(ctx,
        "};\n"
        , NULL);
}

static void qu_struct_init(struct qu_context *ctx, struct qu_class *cls,
    qu_ast_node *node)
{
    struct qu_class_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_struct));
    cls->classdata = self;
    self->body = qu_struct_new_root(ctx);
    qu_visit_struct_children(ctx, node, self->body);
    const char *typename = qu_template_alloc(ctx, "${pref}_${name}",
        "name", cls->name,
        NULL);
    qu_fwdecl_add(ctx, typename, (qu_fwdecl_printer)qu_struct_print, cls);
}

static void qu_struct_func_declare(struct qu_context *ctx,
                                   struct qu_class *cls)
{
    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "struct ${pref}_${typname} *val);\n"
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "struct ${pref}_${typname} *obj, qu_ast_node *node);\n"
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "struct ${pref}_${typname} *obj, int flags);\n"
        , "typname", cls->name
        , NULL);
}
static void qu_struct_func_body(struct qu_context *ctx, struct qu_class *cls)
{
    struct qu_class_struct *self = cls->classdata;
    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "struct ${pref}_${typname} *obj){\n"
        , "typname", cls->name
        , NULL);
    qu_struct_default_setter(ctx, self->body, "obj->");
    qu_code_print(ctx, "}\n\n", NULL);
    qu_code_print(ctx,
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "struct ${pref}_${typname} *obj, qu_ast_node *node0) {\n"
        , "typname", cls->name
        , NULL);
    qu_struct_parser(ctx, self->body, "obj->", 0);
    qu_code_print(ctx, "}\n\n", NULL);
    qu_code_print(ctx,
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "struct ${pref}_${typname} *obj, int flags) {\n"
        , "typname", cls->name
        , NULL);
    qu_struct_printer(ctx, self->body, "obj->");
    qu_code_print(ctx, "}\n\n", NULL);

}
