#include "bool.h"
#include "types.h"
#include "../cli.h"
#include "../context.h"
#include "../../util/parse.h"
#include "../../yaml/access.h"
#include "../../quire_int.h"
#include "../util/print.h"

static void qu_bool_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node);
static struct qu_cli_action *qu_bool_cli_action(struct qu_option *opt,
    const char *action);
static void qu_bool_cli_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action,
    const char *argname);
static void qu_bool_cli_definition(struct qu_context *ctx,
    struct qu_option *opt);
static void qu_bool_cli_apply(struct qu_context *ctx,
    struct qu_option *opt, const char *argname);
static void qu_bool_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level);
static void qu_bool_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname);
static void qu_bool_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag);
static void qu_bool_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);

struct qu_option_vptr qu_bool_vptr = {
    /* parse */ qu_bool_parse,
    /* cli_action */ qu_bool_cli_action,
    /* cli_parser */ qu_bool_cli_parser,
    /* cli_definition */ qu_bool_cli_definition,
    /* cli_apply */ qu_bool_cli_apply,
    /* parse */ qu_bool_parser,
    /* definition */ qu_bool_definition,
    /* printer */ qu_bool_printer,
    /* default_setter */ qu_bool_default_setter
};

struct qu_bool_option {
    unsigned defvalue_set:1;
    int defvalue;
};

static void qu_bool_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node)
{
    struct qu_bool_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_bool_option));
    self->defvalue_set = 0;
    opt->typedata = self;
    opt->typname = "bool";

    if(node->kind == QU_NODE_MAPPING) {

        qu_ast_node *value;

        if((value = qu_map_get(node, "default")) ||
            (value = qu_map_get(node, "="))) {
            opt->has_default = 1;
            self->defvalue_set = 1;
            const char *strvalue = qu_node_content(value);
            if(!qu_parse_bool(strvalue, &self->defvalue))
                qu_err_node_error(ctx->err, value, "Bad boolean value");
        }

    } else if (node->kind == QU_NODE_SCALAR) {
        const char *strvalue = qu_node_content(node);
        if(*strvalue) {  /*  None is allowed as no default  */
            opt->has_default = 1;
            self->defvalue_set = 1;
            if(!qu_parse_bool(strvalue, &self->defvalue))
                qu_err_node_error(ctx->err, node, "Bad boolean value");
        }
    } else {
        qu_err_node_error(ctx->err, node,
            "Boolean type definition must contain either boolean or mapping");
    }
}

static struct qu_cli_action *qu_bool_cli_action(struct qu_option *opt,
    const char *action)
{
    (void) opt;
    static struct qu_cli_action set = {1, "Set ${name:q} (yes/no)", "BOOL"};
    static struct qu_cli_action incr = {0, "Enable ${name:q}", NULL};
    static struct qu_cli_action decr = {0, "Disable ${name:q}", NULL};
    if(action == NULL)  /*  Bare set  */
        return &set;
    if(!strcmp(action, "enable"))
        return &incr;
    if(!strcmp(action, "disable"))
        return &decr;
    return NULL;
}

static void qu_bool_cli_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    (void) opt;
    (void) ctx;
    (void) argname;
    if(action == NULL) {  /*  Bare set  */
        qu_code_print(ctx,
            "if(!qu_parse_bool(${argname}, &cli->${optname:c})) {\n"
            "    qu_optparser_error(ctx, `Boolean expected`);\n"
            "}\n"
            "cli->${optname:c}_set = 1;\n"
            , "argname", argname
            , "optname", opt->path
            , NULL);
        return;
    }
    if(!strcmp(action, "enable")) {
        qu_code_print(ctx,
            "cli->${optname:c} = 1;\n"
            "cli->${optname:c}_set = 1;\n"
            , "optname", opt->path
            , NULL);
        return;
    }
    if(!strcmp(action, "disable")) {
        qu_code_print(ctx,
            "cli->${optname:c} = 0;\n"
            "cli->${optname:c}_set = 1;\n"
            , "optname", opt->path
            , NULL);
        return;
    }
}

static void qu_bool_cli_definition(struct qu_context *ctx,
    struct qu_option *opt)
{
    qu_code_print(ctx,
        "int ${name:c}_set:1;\n"
        "int ${name:c};\n"
        , "name", opt->path
        , NULL);
}
static void qu_bool_cli_apply(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    qu_code_print(ctx,
        "if(cli->${name:c}_set) {\n"
        "   ${expr} = cli->${name:c};\n"
        "}\n"
        , "name", opt->path
        , "expr", expr
        , NULL);
}

static void qu_bool_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level)
{
    (void) opt;
    qu_code_print(ctx,
        "if(!qu_parse_bool(qu_node_content(node${level:d}), &${expr}))\n"
        "   qu_report_error(ctx, node${level:d}, `Bad boolean value`);\n"
        , "level:d", level
        , "expr", expr
        , NULL);

}

static void qu_bool_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname)
{
    (void) opt;
    qu_code_print(ctx,
        "int ${varname:c};\n",
        "varname", varname,
        NULL);
}

static void qu_bool_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag)
{
    (void) opt;
    qu_code_print(ctx,
        "qu_emit_scalar(ctx, ${tag}, NULL, 0, "
                        "(${expr})? `true` : `false`, -1);\n"
        , "tag", tag
        , "expr", expr
        , NULL);
}

static void qu_bool_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    struct qu_bool_option *self = opt->typedata;
    long defvalue = self->defvalue;
    if(!self->defvalue_set) {
        /*  Reset value to zero to be on the safe side  */
        defvalue = 0;
    }
    qu_code_print(ctx,
        "${expr} = ${defvalue:d};\n",
        "expr", expr,
        "defvalue:d", defvalue,
        NULL);
}
