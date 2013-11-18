#include "enum.h"
#include "classes.h"
#include "../struct.h"
#include "../context.h"
#include "../util/print.h"
#include "../types/types.h"
#include "../../util/parse.h"
#include "../../yaml/access.h"

struct qu_enum_option {
    const char *name;
    int value;
};

struct qu_class_enum {
    int options_len;
    int defopt;
    struct qu_enum_option *options;
};

static void qu_enum_init(struct qu_context *, struct qu_class *,
    qu_ast_node *node);
static void qu_enum_var_decl(struct qu_context *ctx, struct qu_class *cls,
    struct qu_option *opt, const char *varname);
static void qu_enum_func_decl(struct qu_context *, struct qu_class *);
static void qu_enum_func_body(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_enum = {
    /* init */ qu_enum_init,
    /* var_decl */ qu_enum_var_decl,
    /* func_decl */ qu_enum_func_decl,
    /* func_body */ qu_enum_func_body
    };

static void qu_enum_print(struct qu_context *ctx, struct qu_class *cls) {
    int i;
    struct qu_class_enum *self = cls->classdata;
    qu_code_print(ctx,
        "enum ${pref}_${typename} {\n"
        , "typename", cls->name
        , NULL);
    for(i = 0; i < self->options_len; ++i) {
        qu_code_print(ctx,
            "${mpref}_${typename:C}_${tagname:C} = ${val:d},\n"
            , "typename", cls->name
            , "tagname", self->options[i].name
            , "val:d", self->options[i].value
            , NULL);
    }
    qu_code_print(ctx,
        "};\n"
        , NULL);
}

static void qu_enum_init(struct qu_context *ctx, struct qu_class *cls,
    qu_ast_node *node)
{
    long tmpval;
    qu_ast_node *tmp;
    struct qu_class_enum *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_enum));
    cls->classdata = self;
    self->options_len = 0;
    self->options = NULL;
    self->defopt = -1;

    qu_ast_node *options = qu_map_get(node, "options");

    if(!options)
        qu_err_node_fatal(ctx->err, node, "Enum must have `options`");

    if(options->kind != QU_NODE_MAPPING)
        qu_err_node_fatal(ctx->err, options, "`options` must be mapping");

    int numopt = 0;
    qu_map_member *mem;
    TAILQ_FOREACH(mem, &options->val.map_index.items, lst)
        numopt += 1;
    if(!numopt)
        qu_err_node_error(ctx->err, options, "At least one option required");
    self->options = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_enum_option)*numopt);
    self->options_len = numopt;
    int i = 0;
    TAILQ_FOREACH(mem, &options->val.map_index.items, lst) {
        self->options[i].name = qu_node_content(mem->key);
        const char *strvalue = qu_node_content(mem->value);
        if(strvalue) {
            if(!qu_parse_int(strvalue, &tmpval))
                qu_err_node_error(ctx->err, mem->value, "Wrong integer");
            self->options[i].value = (int)tmpval;
            i += 1;
        } else {
            qu_err_node_error(ctx->err, mem->value, "Scalar value required");
        }
    }

    if((tmp = qu_map_get(node, "default"))) {
        cls->has_default = 1;
        const char *deftag = qu_node_content(tmp);
        for(i = 0; i < numopt; ++i) {
            if(!strcmp(self->options[i].name, deftag)) {
                self->defopt = self->options[i].value;
                break;
            }
        }
        if(i == numopt) {
            qu_err_node_error(ctx->err, tmp, "Default option is wrong");
        }
    }

    const char *typename = qu_template_alloc(ctx, "enum ${pref}_${name}",
        "name", cls->name,
        NULL);
    qu_fwdecl_add(ctx, typename, (qu_fwdecl_printer)qu_enum_print, cls);
}

static void qu_enum_func_decl(struct qu_context *ctx, struct qu_class *cls)
{
    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "enum ${pref}_${typname} *val);\n"
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "enum ${pref}_${typname} *obj, qu_ast_node *node);\n"
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "enum ${pref}_${typname} *obj, int flags, const char *tag);\n"
        , "typname", cls->name
        , NULL);
}

static void qu_enum_func_body(struct qu_context *ctx, struct qu_class *cls)
{
    int i;
    struct qu_class_enum *self = cls->classdata;

    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "enum ${pref}_${typname} *obj){\n"
        "    *obj = ${defopt:d};\n"
        , "typname", cls->name
        , "defopt:d", self->defopt
        , NULL);
    qu_code_print(ctx, "}\n\n", NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "enum ${pref}_${typname} *obj, qu_ast_node *node0) {\n"
        "const char *val = qu_node_content(node0);\n"
        "if(!val)\n"
        "    qu_report_error(ctx, node0, `Scalar expected`);\n"
        , "typname", cls->name
        , NULL);
    for(i = 0; i < self->options_len; ++i) {
        qu_code_print(ctx,
            "if(!strcmp(val, ${option:q})) {\n"
            "    *obj = ${mpref}_${typename:C}_${option:C};\n"
            "}\n"
            , "typename", cls->name
            , "option", self->options[i].name
            , NULL);
    }
    qu_code_print(ctx, "}\n\n" , NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "enum ${pref}_${typname} *obj, int flags, const char *tag) {\n"
        "    (void) flags;\n"
        "    const char *val = NULL;\n"
        "    switch(*obj) {\n"
        , "typname", cls->name
        , NULL);

    for(i = 0; i < self->options_len; ++i) {
        qu_code_print(ctx,
            "case ${mpref}_${typename:C}_${option:C}:\n"
            "    val = ${option:q};\n"
            "    break;\n"
            , "typename", cls->name
            , "option", self->options[i].name
            , NULL);
    }

    qu_code_print(ctx,
        "}\n"
        "qu_emit_scalar(ctx, tag, NULL, 0, val, -1);\n"
        "}\n\n"
        , NULL);

}

static void qu_enum_var_decl(struct qu_context *ctx, struct qu_class *cls,
    struct qu_option *opt, const char *varname)
{
    (void) cls;
    qu_code_print(ctx,
        "enum ${pref}_${typname} ${varname:c};\n"
        , "typname", opt->typname
        , "varname", varname
        , NULL);
}
