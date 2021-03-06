#include <assert.h>

#include "type.h"
#include "types.h"
#include "../special/types.h"
#include "../classes/classes.h"
#include "../process.h"
#include "../struct.h"
#include "../cli.h"
#include "../context.h"
#include "../../util/parse.h"
#include "../../yaml/access.h"
#include "../../quire_int.h"
#include "../util/print.h"

static void qu_type_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node);
static void qu_type_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level);
static void qu_type_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname);
static void qu_type_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag);
static void qu_type_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);

struct qu_option_vptr qu_type_vptr = {
    /* parse */ qu_type_parse,
    /* cli_action */ NULL,
    /* cli_parser */ NULL,
    /* cli_definition */ NULL,
    /* cli_apply */ NULL,
    /* parse */ qu_type_parser,
    /* definition */ qu_type_definition,
    /* printer */ qu_type_printer,
    /* default_setter */ qu_type_default_setter
};

struct qu_type_option {
    struct qu_class *cls;
    qu_ast_node *defvalue;
};

static void qu_type_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node)
{
    struct qu_type_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_type_option));
    opt->typedata = self;
    self->defvalue = NULL;

    if(node->kind == QU_NODE_MAPPING) {
        qu_ast_node *typnode;
        if((typnode = qu_map_get(node, "type")) ||
            (typnode = qu_map_get(node, "="))) {
            opt->typname = qu_node_content(typnode);
        } else {
            qu_err_node_fatal(ctx->err, node,
                "Type declaration must contain `type` field");
        }
        if((self->defvalue = qu_map_get(node, "default"))) {
            opt->has_default = 1;
        }

    } else if (node->kind == QU_NODE_SCALAR) {
        opt->typname = qu_node_content(node);
    } else {
        qu_err_node_fatal(ctx->err, node,
            "Type declaraiont must contain either string or mapping");
    }
    self->cls = qu_class_get(ctx, opt->typname);
    if(!self->cls) {
        qu_err_node_fatal(ctx->err, node,
            "Type \"%s\" not found", opt->typname);
    }
    if(!opt->has_default) {
        opt->has_default = self->cls->has_default;
    }
}

static void qu_type_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level)
{
    qu_code_print(ctx,
        "${pref}_${typname}_parse(ctx, &${expr}, node${level:d});\n"
        , "level:d", level
        , "typname", opt->typname
        , "expr", expr
        , NULL);
}

static void qu_type_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname)
{
    struct qu_type_option *self = opt->typedata;
    self->cls->vp->var_decl(ctx, self->cls, opt, varname);
}

static void qu_type_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag)
{
    qu_code_print(ctx,
        "${pref}_${typname}_print(ctx, &${expr}, flags, ${tag});\n"
        , "typname", opt->typname
        , "expr", expr
        , "tag", tag
        , NULL);
}

static void qu_type_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    qu_code_print(ctx,
        "${pref}_${typname}_defaults(&${expr});\n"
        , "typname", opt->typname
        , "expr", expr
        , NULL);
}
