#include "str.h"
#include "types.h"
#include "../cli.h"
#include "../context.h"
#include "../../util/parse.h"
#include "../../yaml/access.h"
#include "../../quire_int.h"
#include "../util/print.h"

static void qu_str_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node);
static struct qu_cli_action *qu_str_cli_action(struct qu_option *opt,
    const char *action);
static void qu_str_cli_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname);
static void qu_str_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *node);
static void qu_str_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname);
static void qu_str_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);
static void qu_str_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);

struct qu_option_vptr qu_str_vptr = {
    /* parse */ qu_str_parse,
    /* cli_action */ qu_str_cli_action,
    /* cli_parser */ qu_str_cli_parser,
    /* parse */ qu_str_parser,
    /* definition */ qu_str_definition,
    /* printer */ qu_str_printer,
    /* default_setter */ qu_str_default_setter
};

struct qu_str_option {
    unsigned defvalue_set:1;
    const char *defvalue;
};

static void qu_str_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node)
{
    struct qu_str_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_str_option));
    self->defvalue_set = 0;
    self->defvalue = NULL;
    opt->typedata = self;

    if(node->kind == QU_NODE_MAPPING) {

        qu_ast_node *value;

        if((value = qu_map_get(node, "default")) ||
            (value = qu_map_get(node, "="))) {
            opt->has_default = 1;
            self->defvalue_set = 1;
            self->defvalue = qu_node_content(value);
        }

    } else if (node->kind == QU_NODE_SCALAR) {
        const char *strvalue = qu_node_content(node);
        if(*strvalue) {  /*  None is allowed as no default  */
            opt->has_default = 1;
            self->defvalue_set = 1;
            self->defvalue = strvalue;
        }
    } else {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
            "String type must contain either string or mapping");
    }
}

static struct qu_cli_action *qu_str_cli_action(struct qu_option *opt,
    const char *action)
{
    static struct qu_cli_action set = {1, "Set ${name:q}", "STR"};
    if(action == NULL)  /*  Bare set  */
        return &set;
    return NULL;
}

static void qu_str_cli_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    if(action == NULL) {  /*  Bare set  */
        return;
    }
}

static void qu_str_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *node)
{
    qu_code_print(ctx,
        "qu_node_to_str(ctx, ${node}, &${expr}, &${expr}_len);\n",
        // TODO(tailhook) check min and max
        "node", node,
        "expr", expr,
        NULL);

}

static void qu_str_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname)
{
    qu_code_print(ctx,
        "const char *${varname:c};\n"
        "int ${varname:c}_len;\n",
        "varname", varname,
        NULL);
}

static void qu_str_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    qu_code_print(ctx,
        "qu_emit_scalar(ctx, NULL, NULL, 0, ${expr}, ${expr}_len);\n",
        "expr", expr,
        NULL);
}

static void qu_str_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    struct qu_str_option *self = opt->typedata;
    if(self->defvalue_set) {
        qu_code_print(ctx,
            "${expr} = ${defvalue:q};\n"
            "${expr}_len = ${deflen:d};\n",
            "expr", expr,
            "defvalue", self->defvalue,
            "deflen:d", strlen(self->defvalue),
            NULL);
    } else {
        qu_code_print(ctx,
            "${expr} = NULL;\n"
            "${expr}_len = 0;\n",
            "expr", expr,
            NULL);
    }
}
