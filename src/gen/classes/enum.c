#include "enum.h"
#include "classes.h"
#include "../struct.h"
#include "../context.h"
#include "../util/print.h"
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
static void qu_enum_func_body(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_enum = {
    /* init */ qu_enum_init,
    /* func_body */ qu_enum_func_body
    };

static void qu_enum_print(struct qu_context *ctx, struct qu_class *cls) {
    int i;
    struct qu_class_enum *self = cls->classdata;
    qu_code_print(ctx,
        "struct ${pref}_${typename} {\n"
        "    enum ${pref}_${typename}_enum {\n"
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
        "} val;\n"
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
        LONGJUMP_ERR_NODE(ctx, node, "Enum must have `options`");

    if(options->kind != QU_NODE_MAPPING)
        LONGJUMP_ERR_NODE(ctx, options, "`options` must be mapping");

    int numopt = 0;
    qu_map_member *mem;
    TAILQ_FOREACH(mem, &options->val.map_index.items, lst)
        numopt += 1;
    if(!numopt)
        LONGJUMP_ERR_NODE(ctx, options, "At least one option required");
    self->options = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_enum_option)*numopt);
    self->options_len = numopt;
    int i = 0;
    TAILQ_FOREACH(mem, &options->val.map_index.items, lst) {
        self->options[i].name = qu_node_content(mem->key);
        const char *strvalue = qu_node_content(mem->value);
        if(!strvalue)
            LONGJUMP_ERR_NODE(ctx, mem->value, "Scalar value required");
        const char *end = qu_parse_int(strvalue, &tmpval);
        if(end != strvalue + strlen(strvalue))
            LONGJUMP_ERR_NODE(ctx, mem->value, "Wrong numeric value");
        self->options[i].value = (int)tmpval;
        i += 1;
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
            LONGJUMP_ERR_NODE(ctx, node, "Default option is wrong");
        }
    }

    const char *typename = qu_template_alloc(ctx, "${pref}_${name}",
        "name", cls->name,
        NULL);
    qu_fwdecl_add(ctx, typename, (qu_fwdecl_printer)qu_enum_print, cls);
}

static void qu_enum_func_body(struct qu_context *ctx, struct qu_class *cls)
{
    int i;
    struct qu_class_enum *self = cls->classdata;

    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "struct ${pref}_${typname} *obj){\n"
        "    obj->val = ${defopt:d};\n"
        , "typname", cls->name
        , "defopt:d", self->defopt
        , NULL);
    qu_code_print(ctx, "}\n\n", NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "struct ${pref}_${typname} *obj, qu_ast_node *node0) {\n"
        "const char *val = qu_node_content(node0);\n"
        "if(!val)\n"
        "    qu_report_error(ctx, node0, `Scalar expected`);\n"
        , "typname", cls->name
        , NULL);
    for(i = 0; i < self->options_len; ++i) {
        qu_code_print(ctx,
            "if(!strcmp(val, ${option:q})) {\n"
            "    obj->val = ${mpref}_${typename:C}_${option:C};\n"
            "}\n"
            , "typename", cls->name
            , "option", self->options[i].name
            , NULL);
    }
    qu_code_print(ctx, "}\n\n" , NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "struct ${pref}_${typname} *obj, int flags) {\n"
        "    const char *val = NULL;\n"
        "    switch(obj->val) {\n"
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
        "qu_emit_scalar(ctx, NULL, NULL, 0, val, -1);\n"
        "}\n\n"
        , NULL);

}
