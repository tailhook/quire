#include "type.h"
#include "types.h"
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
    struct qu_option *opt, const char *expr);
static void qu_type_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);

struct qu_option_vptr qu_type_vptr = {
    /* parse */ qu_type_parse,
    /* cli_action */ NULL,
    /* cli_parser */ NULL,
    /* parse */ qu_type_parser,
    /* definition */ qu_type_definition,
    /* printer */ qu_type_printer,
    /* default_setter */ qu_type_default_setter
};

struct qu_type_option {
    qu_ast_node *defvalue;
};

static void qu_type_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node)
{
    struct qu_type_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_type_option));
    opt->typedata = self;
    self->defvalue = node;

    if(node->kind == QU_NODE_MAPPING) {
        qu_ast_node *typnode;
        if((typnode = qu_map_get(node, "type")) ||
            (typnode = qu_map_get(node, "="))) {
            opt->typname = qu_node_content(typnode);
        } else {
            LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
                "Type declaration must contain `type` field");
        }
        if((self->defvalue = qu_map_get(node, "default"))) {
            opt->has_default = 1;
        }

    } else if (node->kind == QU_NODE_SCALAR) {
        opt->typname = qu_node_content(node);
    } else {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
            "Type declaraiont must contain either string or mapping");
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
    qu_code_print(ctx,
        "struct ${pref}_${typname} ${varname:c};\n"
        , "typname", opt->typname
        , "varname", varname
        , NULL);
}

static void qu_type_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    qu_code_print(ctx,
        "${pref}_${typname}_print(ctx, &${expr}, flags);\n"
        , "typname", opt->typname
        , "expr", expr
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
