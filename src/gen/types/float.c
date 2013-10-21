#include "float.h"
#include "types.h"
#include "../cli.h"
#include "../context.h"
#include "../../util/parse.h"
#include "../../yaml/access.h"
#include "../../quire_int.h"
#include "../util/print.h"

static void qu_float_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node);
static struct qu_cli_action *qu_float_cli_action(struct qu_option *opt,
    const char *action);
static void qu_float_cli_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname);
static void qu_float_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level);
static void qu_float_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname);
static void qu_float_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag);
static void qu_float_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);

struct qu_option_vptr qu_float_vptr = {
    /* parse */ qu_float_parse,
    /* cli_action */ qu_float_cli_action,
    /* cli_parser */ qu_float_cli_parser,
    /* parse */ qu_float_parser,
    /* definition */ qu_float_definition,
    /* printer */ qu_float_printer,
    /* default_setter */ qu_float_default_setter
};

struct qu_float_option {
    unsigned defvalue_set:1;
    double defvalue;
};

static void qu_float_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node)
{
    struct qu_float_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_float_option));
    self->defvalue_set = 0;
    opt->typedata = self;
    opt->typname = "int";

    if(node->kind == QU_NODE_MAPPING) {

        qu_ast_node *value;

        if((value = qu_map_get(node, "default")) ||
            (value = qu_map_get(node, "="))) {
            opt->has_default = 1;
            self->defvalue_set = 1;
            const char *strvalue = qu_node_content(value);
            const char *end = qu_parse_float(strvalue, &self->defvalue);
            if(end != strvalue + strlen(strvalue))
                LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, value->start_token,
                    "Bad integer value")
        }

    } else if (node->kind == QU_NODE_SCALAR) {
        const char *strvalue = qu_node_content(node);
        if(*strvalue) {  /*  None is allowed as no default  */
            opt->has_default = 1;
            self->defvalue_set = 1;
            const char *end = qu_parse_float(strvalue, &self->defvalue);
            if(end != strvalue + strlen(strvalue))
                LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
                    "Bad integer value")
        }
    } else {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
            "Int type must contain either integer or mapping");
    }
}

static struct qu_cli_action *qu_float_cli_action(struct qu_option *opt,
    const char *action)
{
    static struct qu_cli_action set = {1, "Set ${name:q}", "FLOAT"};
    if(action == NULL)  /*  Bare set  */
        return &set;
    return NULL;
}

static void qu_float_cli_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    if(action == NULL) {  /*  Bare set  */
        return;
    }
}

static void qu_float_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level)
{
    qu_code_print(ctx,
        "qu_node_to_float(ctx, node${level:d}, &${expr});\n",
        "level:d", level,
        "expr", expr,
        NULL);

}

static void qu_float_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname)
{
    qu_code_print(ctx,
        "double ${varname:c};\n",
        "varname", varname,
        NULL);
}

static void qu_float_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag)
{
    qu_code_print(ctx,
        "char buf[64];\n"
        "int vlen = sprintf(buf, `%g`, ${expr});\n"
        "qu_emit_scalar(ctx, ${tag}, NULL, 0, buf, vlen);\n"
        , "tag", tag
        , "expr", expr
        , NULL);
}

static void qu_float_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    struct qu_float_option *self = opt->typedata;
    double defvalue = self->defvalue;
    if(!self->defvalue_set) {
        /*  Reset value to zero to be on the safe side  */
        defvalue = 0;
    }
    qu_code_print(ctx,
        "${expr} = ${defvalue:g};\n",
        "expr", expr,
        "defvalue:g", defvalue,
        NULL);
}
