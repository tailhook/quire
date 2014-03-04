#include <assert.h>

#include "field.h"
#include "classes.h"
#include "../struct.h"
#include "../types/types.h"
#include "../context.h"
#include "../util/print.h"
#include "../../util/parse.h"
#include "../process.h"
#include "../../yaml/access.h"

struct qu_class_field {
    struct qu_option *opt;
};

static void qu_field_init(struct qu_context *, struct qu_class *,
    qu_ast_node *node);
static void qu_field_var_decl(struct qu_context *, struct qu_class *,
    struct qu_option *opt, const char *varname);
static void qu_field_func_decl(struct qu_context *, struct qu_class *);
static void qu_field_func_body(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_field = {
    /* init */ qu_field_init,
    /* var_decl */ qu_field_var_decl,
    /* func_decl */ qu_field_func_decl,
    /* func_body */ qu_field_func_body
    };

static void qu_field_print(struct qu_context *ctx, struct qu_class *cls) {
    struct qu_class_field *self = cls->classdata;
    qu_code_print(ctx,
        "struct ${pref}_${typename} {\n"
        , "typename", cls->name
        , NULL);
    self->opt->vp->definition(ctx, self->opt, "val");
    qu_classes_print_cdecls(ctx, cls->node);
    qu_code_print(ctx,
        "};\n"
        , NULL);
}

static void qu_field_init(struct qu_context *ctx, struct qu_class *cls,
    qu_ast_node *node)
{
    struct qu_class_field *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_field));
    cls->classdata = self;

    qu_ast_node *field = qu_map_get(node, "field");

    if(!field)
        qu_err_node_fatal(ctx->err, node, "Field must have `field` key");

    self->opt = qu_parse_option(ctx, field, "", NULL);

    const char *typename = qu_template_alloc(ctx, "struct ${pref}_${name}",
        "name", cls->name,
        NULL);
    qu_fwdecl_add(ctx, typename, (qu_fwdecl_printer)qu_field_print, cls);
}

static void qu_field_func_decl(struct qu_context *ctx, struct qu_class *cls)
{
    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "struct ${pref}_${typname} *val);\n"
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "struct ${pref}_${typname} *obj, qu_ast_node *node);\n"
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "struct ${pref}_${typname} *obj, int flags, const char *tag);\n"
        , "typname", cls->name
        , NULL);
}

static void qu_field_func_body(struct qu_context *ctx, struct qu_class *cls)
{
    struct qu_class_field *self = cls->classdata;

    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "struct ${pref}_${typname} *obj){\n"
        , "typname", cls->name
        , NULL);
    self->opt->vp->default_setter(ctx, self->opt, "obj->val");
    qu_code_print(ctx, "}\n\n", NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "struct ${pref}_${typname} *obj, qu_ast_node *node0) {\n"
        , "typname", cls->name
        , NULL);
    self->opt->vp->parser(ctx, self->opt, "obj->val", 0);
    qu_code_print(ctx, "}\n\n" , NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "struct ${pref}_${typname} *obj, int flags, const char *tag) {\n"
        "    (void) flags; \n"
        , "typname", cls->name
        , NULL);

    self->opt->vp->printer(ctx, self->opt, "obj->val", "tag");
    qu_code_print(ctx, "}\n\n", NULL);

}

static void qu_field_var_decl(struct qu_context *ctx, struct qu_class *cls,
    struct qu_option *opt, const char *varname)
{
    (void) cls;
    qu_code_print(ctx,
        "struct ${pref}_${typname} ${varname:c};\n"
        , "typname", opt->typname
        , "varname", varname
        , NULL);
}
