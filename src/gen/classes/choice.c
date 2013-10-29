#include "choice.h"
#include "classes.h"
#include "../struct.h"
#include "../context.h"
#include "../../quire_int.h"
#include "../../yaml/access.h"
#include "../types/types.h"
#include "../process.h"

struct qu_choice {
    const char *name;
    const char *prefix;
    struct qu_option *opt;
};

struct qu_class_choice {
    int choices_len;
    struct qu_choice *choices;
};

static void qu_choice_init(struct qu_context *, struct qu_class *,
    qu_ast_node *node);
static void qu_choice_var_decl(struct qu_context *, struct qu_class *,
    struct qu_option *opt, const char *varname);
static void qu_choice_func_decl(struct qu_context *, struct qu_class *);
static void qu_choice_func_body(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_choice = {
    /* init */ qu_choice_init,
    /* var_decl */ qu_choice_var_decl,
    /* func_decl */ qu_choice_func_decl,
    /* func_body */ qu_choice_func_body
    };

static void qu_choice_print(struct qu_context *ctx, struct qu_class *cls) {
    int i;
    struct qu_class_choice *self = cls->classdata;
    qu_code_print(ctx,
        "enum ${pref}_${typename}_choice {\n"
        , "typename", cls->name
        , NULL);
    for(i = 0; i < self->choices_len; ++i) {
        qu_code_print(ctx,
            "${mpref}_${typename:C}_${tagname:C} = ${val:d},\n"
            , "typename", cls->name
            , "tagname", self->choices[i].name
            , "val:d", i+1
            , NULL);
    }
    qu_code_print(ctx,
        "};\n"
        "\n"
        "union ${pref}_${typename} {\n"
        "   struct ${pref}_${typename}_any {\n"
        "       enum ${pref}_${typename}_choice tag;\n"
        "   } any;\n"
        , "typename", cls->name
        , NULL);
    for(i = 0; i < self->choices_len; ++i) {
        qu_code_print(ctx,
            "struct ${pref}_${typename}_${choice:c} {\n"
            "   enum ${pref}_${typename}_choice tag;\n"
            , "typename", cls->name
            , "choice", self->choices[i].name
            , NULL);
        self->choices[i].opt->vp->definition(ctx, self->choices[i].opt, "val");
        qu_code_print(ctx,
            "} ${choice:c};\n"
            , "choice", self->choices[i].name
            , NULL);
    }
    qu_code_print(ctx,
        "};\n"
        "\n"
        , NULL);
}

static void qu_choice_init(struct qu_context *ctx, struct qu_class *cls,
    qu_ast_node *node)
{
    struct qu_class_choice *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_choice));
    cls->classdata = self;
    self->choices_len = 0;

    qu_ast_node *choices = qu_map_get(node, "choices");

    if(!choices || choices->kind != QU_NODE_MAPPING)
        LONGJUMP_ERR_NODE(ctx, node, "Choice should contain mapping `choices`");

    int numch = 0;
    qu_map_member *mem;
    TAILQ_FOREACH(mem, &choices->val.map_index.items, lst)
        numch += 1;
    if(!numch)
        LONGJUMP_ERR_NODE(ctx, choices, "At least one choice required");

    self->choices = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_choice)*numch);
    self->choices_len = numch;
    int i = 0;
    TAILQ_FOREACH(mem, &choices->val.map_index.items, lst) {
        self->choices[i].name = qu_node_content(mem->key);
        self->choices[i].prefix = qu_template_alloc(ctx,
            "(*obj)->${tagname:c}.val"
            , "tagname", self->choices[i].name
            , NULL);
        self->choices[i].opt = qu_parse_option(ctx, mem->value, "", NULL);
        i += 1;
    }

    const char *typename = qu_template_alloc(ctx, "union ${pref}_${name}",
        "name", cls->name,
        NULL);
    qu_fwdecl_add(ctx, typename, (qu_fwdecl_printer)qu_choice_print, cls);
}

static void qu_choice_func_decl(struct qu_context *ctx, struct qu_class *cls)
{
    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "union ${pref}_${typname} **val);\n"
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "union ${pref}_${typname} **obj, qu_ast_node *node);\n"
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "union ${pref}_${typname} **obj, int flags, const char *tag);\n"
        , "typname", cls->name
        , NULL);
}

static void qu_choice_func_body(struct qu_context *ctx, struct qu_class *cls)
{
    int i;
    struct qu_class_choice *self = cls->classdata;

    qu_code_print(ctx,
        "static void ${pref}_${typname:c}_defaults("
            "union ${pref}_${typname:c} **obj){\n"
        "    *obj = NULL;\n"
        , "typname", cls->name
        , NULL);
    qu_code_print(ctx, "}\n\n", NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "union ${pref}_${typname:c} **obj, qu_ast_node *node0) {\n"
        "const char *nodetag = qu_node_tag(node0);\n"
        "if(nodetag) {\n"
        , "typname", cls->name
        , NULL);
    for(i = 0; i < self->choices_len; ++i) {
        qu_code_print(ctx,
            "if(!strcmp(nodetag, `!` ${tagname:q})) {\n"
            "    *obj = qu_config_alloc(ctx, "
                "sizeof(struct ${pref}_${typename:c}_${tagname:c}));\n"
            "    (*obj)->${tagname:c}.tag = ${mpref}_${typename:C}_${tagname:C};\n"
            , "typename", cls->name
            , "tagname", self->choices[i].name
            , NULL);
        self->choices[i].opt->vp->default_setter(ctx,
            self->choices[i].opt, self->choices[i].prefix);
        self->choices[i].opt->vp->parser(ctx,
            self->choices[i].opt, self->choices[i].prefix, 0);
        qu_code_print(ctx,
            "return;\n"
            "}\n"
            , NULL);
    }
    qu_code_print(ctx,
            /*  Allow empty (null) value  */
            "qu_report_error(ctx, node0, `Wrong tag`);\n"
            "} else if(strcmp(qu_node_content(node0), ``)) {\n"
            "   qu_report_error(ctx, node0, `Tag is expected`);\n"
            "}\n"
            , NULL);
    qu_code_print(ctx, "}\n\n" , NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "union ${pref}_${typname} **obj, int flags, const char *tag) {\n"
        "    if(!*obj) {\n"
        "        qu_emit_scalar(ctx, tag, NULL, 0, ``, 0);\n"
        "    } else {\n"
        "    switch((*obj)->any.tag) {\n"
        , "typname", cls->name
        , NULL);

    for(i = 0; i < self->choices_len; ++i) {
        qu_code_print(ctx,
            "case ${mpref}_${typename:C}_${tagname:C}:\n"
            "    tag = `!` ${tagname:q};\n"
            , "typename", cls->name
            , "tagname", self->choices[i].name
            , NULL);
        self->choices[i].opt->vp->printer(ctx,
            self->choices[i].opt, self->choices[i].prefix, "tag");
        qu_code_print(ctx,
            "    break;\n"
            , NULL);
    }

    qu_code_print(ctx, "}\n", NULL);
    qu_code_print(ctx, "}\n", NULL);
    qu_code_print(ctx, "}\n\n", NULL);
}

static void qu_choice_var_decl(struct qu_context *ctx, struct qu_class *cls,
    struct qu_option *opt, const char *varname) {
    qu_code_print(ctx,
        "union ${pref}_${typname} *${varname:c};\n"
        , "typname", opt->typname
        , "varname", varname
        , NULL);
}
