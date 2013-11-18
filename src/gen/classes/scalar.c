#include <assert.h>

#include "scalar.h"
#include "classes.h"
#include "../struct.h"
#include "../types/types.h"
#include "../context.h"
#include "../util/print.h"
#include "../../util/parse.h"
#include "../process.h"
#include "../../yaml/access.h"

struct qu_scalar_tag {
    const char *name;
    int value;
};

struct qu_class_scalar {
    int tags_len;
    struct qu_scalar_tag *tags;
    struct qu_option *opt;
    int deftag;
};

static void qu_scalar_init(struct qu_context *, struct qu_class *,
    qu_ast_node *node);
static void qu_scalar_var_decl(struct qu_context *, struct qu_class *,
    struct qu_option *opt, const char *varname);
static void qu_scalar_func_decl(struct qu_context *, struct qu_class *);
static void qu_scalar_func_body(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_scalar = {
    /* init */ qu_scalar_init,
    /* var_decl */ qu_scalar_var_decl,
    /* func_decl */ qu_scalar_func_decl,
    /* func_body */ qu_scalar_func_body
    };

static void qu_scalar_print(struct qu_context *ctx, struct qu_class *cls) {
    int i;
    struct qu_class_scalar *self = cls->classdata;
    qu_code_print(ctx,
        "struct ${pref}_${typename} {\n"
        "    enum ${pref}_${typename}_tag {\n"
        , "typename", cls->name
        , NULL);
    if(self->deftag < 0) {
        qu_code_print(ctx,
            "${mpref}_${typename:C}_UNDEF = ${deftag:d}, \n"
            , "deftag:d", self->deftag
            , "typename", cls->name
            , NULL);
    }
    for(i = 0; i < self->tags_len; ++i) {
        qu_code_print(ctx,
            "${mpref}_${typename:C}_${tagname:C} = ${val:d},\n"
            , "typename", cls->name
            , "tagname", self->tags[i].name
            , "val:d", self->tags[i].value
            , NULL);
    }
    qu_code_print(ctx,
        "} tag;\n"
        , NULL);
    self->opt->vp->definition(ctx, self->opt, "val");
    qu_code_print(ctx,
        "};\n"
        , NULL);
}

static void qu_scalar_init(struct qu_context *ctx, struct qu_class *cls,
    qu_ast_node *node)
{
    long tmpval;
    qu_ast_node *tmp;
    struct qu_class_scalar *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_scalar));
    cls->classdata = self;
    self->tags_len = 0;
    self->tags = NULL;
    self->deftag = -1;

    qu_ast_node *tags = qu_map_get(node, "tags");
    qu_ast_node *type = qu_map_get(node, "type");

    if(!tags || !type)
        qu_err_node_fatal(ctx->err, node,
            "TagScalar must have `type` and `tags`");

    if(tags->kind != QU_NODE_MAPPING)
        qu_err_node_fatal(ctx->err, tags, "`tags` must be mapping");

    int numtags = 0;
    qu_map_member *mem;
    TAILQ_FOREACH(mem, &tags->val.map_index.items, lst)
        numtags += 1;
    if(!numtags)
        qu_err_node_error(ctx->err, tags, "At least one tag required");
    self->tags = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_scalar_tag)*numtags);
    self->tags_len = numtags;
    int i = 0;
    TAILQ_FOREACH(mem, &tags->val.map_index.items, lst) {
        self->tags[i].name = qu_node_content(mem->key);
        const char *strvalue = qu_node_content(mem->value);
        if(strvalue) {
            if(!qu_parse_int(strvalue, &tmpval))
                qu_err_node_error(ctx->err, mem->value, "Wrong numeric value");
            self->tags[i].value = (int)tmpval;
            i += 1;
        } else {
            qu_err_node_error(ctx->err, mem->value, "Scalar value required");
        }
    }

    if((tmp = qu_map_get(node, "default-tag"))) {
        const char *deftag = qu_node_content(tmp);
        for(i = 0; i < numtags; ++i) {
            if(!strcmp(self->tags[i].name, deftag)) {
                self->deftag = self->tags[i].value;
                break;
            }
        }
        if(i == numtags) {
            qu_err_node_error(ctx->err, tmp, "Default tag is wrong");
        }
    }

    self->opt = qu_parse_option(ctx, qu_map_get(node, "type"), "", NULL);

    const char *typename = qu_template_alloc(ctx, "struct ${pref}_${name}",
        "name", cls->name,
        NULL);
    qu_fwdecl_add(ctx, typename, (qu_fwdecl_printer)qu_scalar_print, cls);
}

static void qu_scalar_func_decl(struct qu_context *ctx, struct qu_class *cls)
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

static void qu_scalar_func_body(struct qu_context *ctx, struct qu_class *cls)
{
    int i;
    struct qu_class_scalar *self = cls->classdata;

    qu_code_print(ctx,
        "static void ${pref}_${typname}_defaults("
            "struct ${pref}_${typname} *obj){\n"
        "    obj->tag = ${deftag:d};\n"
        , "typname", cls->name
        , "deftag:d", self->deftag
        , NULL);
    self->opt->vp->default_setter(ctx, self->opt, "obj->val");
    qu_code_print(ctx, "}\n\n", NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_parse("
            "struct qu_config_context *ctx, "
            "struct ${pref}_${typname} *obj, qu_ast_node *node0) {\n"
        "const char *nodetag = qu_node_tag(node0);\n"
        "if(nodetag) {\n"
        , "typname", cls->name
        , NULL);
    for(i = 0; i < self->tags_len; ++i) {
        qu_code_print(ctx,
            "if(!strcmp(nodetag, `!` ${tagname:q})) {\n"
            "    obj->tag = ${val:d};\n"
            "}\n"
            , "typename", cls->name
            , "val:d", self->tags[i].value
            , "tagname", self->tags[i].name
            , NULL);
    }
    qu_code_print(ctx, "}\n" , NULL);
    if(self->deftag < 0) {
        qu_code_print(ctx,
            "if(obj->tag < 0) {\n"
            "    qu_report_error(ctx, node0, `Wrong or absent tag`);\n"
            "}\n"
            , NULL);
    }
    self->opt->vp->parser(ctx, self->opt, "obj->val", 0);
    qu_code_print(ctx, "}\n\n" , NULL);

    qu_code_print(ctx,
        "static void ${pref}_${typname}_print("
            "struct qu_emit_context *ctx, "
            "struct ${pref}_${typname} *obj, int flags, const char *tag) {\n"
        "    (void) flags; \n"
        "    switch(obj->tag) {\n"
        , "typname", cls->name
        , NULL);

    for(i = 0; i < self->tags_len; ++i) {
        qu_code_print(ctx,
            "case ${mpref}_${typename:C}_${tagname:C}:\n"
            "    tag = `!` ${tagname:q};\n"
            "    break;\n"
            , "typename", cls->name
            , "tagname", self->tags[i].name
            , NULL);
    }

    qu_code_print(ctx,
        "default:\n"
        "   qu_emit_scalar(ctx, tag, NULL, 0, ``, 0);\n"
        "   return;\n"
        "}\n\n"
        , NULL);
    self->opt->vp->printer(ctx, self->opt, "obj->val", "tag");
    qu_code_print(ctx, "}\n\n", NULL);

}

static void qu_scalar_var_decl(struct qu_context *ctx, struct qu_class *cls,
    struct qu_option *opt, const char *varname)
{
    (void) cls;
    qu_code_print(ctx,
        "struct ${pref}_${typname} ${varname:c};\n"
        , "typname", opt->typname
        , "varname", varname
        , NULL);
}
