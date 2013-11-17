#include "int.h"
#include "types.h"
#include "../cli.h"
#include "../context.h"
#include "../../util/parse.h"
#include "../../yaml/access.h"
#include "../../quire_int.h"
#include "../util/print.h"

static void qu_int_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node);
static struct qu_cli_action *qu_int_cli_action(struct qu_option *opt,
    const char *action);
static void qu_int_cli_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname);
static void qu_int_cli_definition(struct qu_context *ctx,
    struct qu_option *opt);
static void qu_int_cli_apply(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);
static void qu_int_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level);
static void qu_int_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname);
static void qu_int_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag);
static void qu_int_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);

struct qu_option_vptr qu_int_vptr = {
    /* parse */ qu_int_parse,
    /* cli_action */ qu_int_cli_action,
    /* cli_parser */ qu_int_cli_parser,
    /* cli_definition */ qu_int_cli_definition,
    /* cli_apply */ qu_int_cli_apply,
    /* parse */ qu_int_parser,
    /* definition */ qu_int_definition,
    /* printer */ qu_int_printer,
    /* default_setter */ qu_int_default_setter
};

struct qu_int_option {
    unsigned defvalue_set:1;
    unsigned min_set:1;
    unsigned max_set:1;
    long defvalue;
    long min;
    long max;
};

static void qu_int_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node)
{
    struct qu_int_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_int_option));
    self->defvalue_set = 0;
    self->min_set = 0;
    self->max_set = 0;
    opt->typedata = self;
    opt->typname = "int";

    if(node->kind == QU_NODE_MAPPING) {

        qu_ast_node *value;

        if((value = qu_map_get(node, "default")) ||
            (value = qu_map_get(node, "="))) {
            opt->has_default = 1;
            self->defvalue_set = 1;
            const char *strvalue = qu_node_content(value);
            const char *end = qu_parse_int(strvalue, &self->defvalue);
            if(end != strvalue + strlen(strvalue))
                qu_err_node_error(ctx->err, value, "Bad integer value");
        }
        if((value = qu_map_get(node, "min"))) {
            self->min_set = 1;
            const char *strvalue = qu_node_content(value);
            const char *end = qu_parse_int(strvalue, &self->min);
            if(end != strvalue + strlen(strvalue))
                qu_err_node_error(ctx->err, value, "Bad integer value");
        }

        if((value = qu_map_get(node, "max"))) {
            self->max_set = 1;
            const char *strvalue = qu_node_content(value);
            const char *end = qu_parse_int(strvalue, &self->max);
            if(end != strvalue + strlen(strvalue))
                qu_err_node_error(ctx->err, value, "Bad integer value");
        }

    } else if (node->kind == QU_NODE_SCALAR) {
        const char *strvalue = qu_node_content(node);
        if(*strvalue) {  /*  None is allowed as no default  */
            opt->has_default = 1;
            self->defvalue_set = 1;
            const char *end = qu_parse_int(strvalue, &self->defvalue);
            if(end != strvalue + strlen(strvalue))
                qu_err_node_error(ctx->err, node, "Bad integer value");
        }
    } else {
        qu_err_node_error(ctx->err, node,
            "Int type definition must contain either integer or mapping");
    }
}

static struct qu_cli_action *qu_int_cli_action(struct qu_option *opt,
    const char *action)
{
    (void) opt;
    static struct qu_cli_action set = {1, "Set ${name:q}", "INT"};
    static struct qu_cli_action incr = {0, "Increment ${name:q}", NULL};
    static struct qu_cli_action decr = {0, "Decrement ${name:q}", NULL};
    if(action == NULL)  /*  Bare set  */
        return &set;
    if(!strcmp(action, "incr"))
        return &incr;
    if(!strcmp(action, "decr"))
        return &decr;
    return NULL;
}

static void qu_int_cli_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    struct qu_int_option *self = opt->typedata;
    if(action == NULL) {  /*  Bare set  */
        qu_code_print(ctx,
            "char *end;\n"
            "long val = strtol(${argname}, &end, 0);\n"
            "if(end != ${argname} + strlen(${argname})) {\n"
            "    qu_optparser_error(ctx, `Integer expected`);\n"
            "}\n"
            , "argname", argname
            , "optname", opt->path
            , NULL);
        if(self->min) {
            qu_code_print(ctx,
                "if(val < ${min:l})\n"
                "    qu_optparser_error(ctx, "
                    "`Integer too low, min ${min:l}`);\n"
                , "min:l", self->min
                , NULL);
        }
        if(self->max) {
            qu_code_print(ctx,
                "if(val > ${max:d})\n"
                "    qu_optparser_error(ctx, "
                    "`Integer too big, max ${max:d}`);\n"
                , "max:l", self->max
                , NULL);
        }

        qu_code_print(ctx,
            "cli->${optname:c}_set = 1;\n"
            "cli->${optname:c} = val;\n"
            , "argname", argname
            , "optname", opt->path
            , NULL);
        return;
    }
    if(!strcmp(action, "incr")) {
        qu_code_print(ctx,
            "cli->${optname:c}_delta_set = 1;\n"
            "cli->${optname:c}_delta += 1;\n"
            , "argname", argname
            , "optname", opt->path
            , NULL);
        return;
    }
    if(!strcmp(action, "decr")) {
        qu_code_print(ctx,
            "cli->${optname:c}_delta_set = 1;\n"
            "cli->${optname:c}_delta -= 1;\n"
            , "argname", argname
            , "optname", opt->path
            , NULL);
        return;
    }
}

static void qu_int_cli_definition(struct qu_context *ctx,
    struct qu_option *opt)
{
    qu_code_print(ctx,
        "int ${name:c}_set:1;\n"
        "int ${name:c}_delta_set:1;\n"
        "long ${name:c};\n"
        "long ${name:c}_delta;\n"
        , "name", opt->path
        , NULL);
}

static void qu_int_cli_apply(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    struct qu_int_option *self = (struct qu_int_option *)opt->typedata;
    qu_code_print(ctx,
        "if(cli->${name:c}_set) {\n"
        "   ${expr} = cli->${name:c};\n"
        "}\n"
        "if(cli->${name:c}_delta_set) {\n"
        "   ${expr} += cli->${name:c}_delta;\n"
        , "name", opt->path
        , "expr", expr
        , NULL);
    if(self->min) {
        qu_code_print(ctx,
            "if(${expr} < ${min:l})\n"
            "    ${expr} = ${min:l};\n"
            , "min:l", self->min
            , "expr", expr
            , NULL);
    }
    if(self->max) {
        qu_code_print(ctx,
            "if(${expr} > ${max:l})\n"
            "    ${expr} = ${max:l};\n"
            , "max:l", self->max
            , "expr", expr
            , NULL);
    }
    qu_code_print(ctx,
        "}\n"
        , NULL);
}

static void qu_int_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level)
{
    struct qu_int_option *self = (struct qu_int_option *)opt->typedata;
    qu_code_print(ctx,
        "qu_node_to_int(ctx, node${level:d}, &${expr});\n"
        , "level:d", level
        , "expr", expr
        , NULL);

    if(self->min) {
        qu_code_print(ctx,
            "if(${expr} < ${min:l})\n"
            "    qu_report_error(ctx, node${level:d}, "
                "`Integer too low, min ${min:l}`);\n"
            , "min:l", self->min
            , "level:d", level
            , "expr", expr
            , NULL);
    }
    if(self->max) {
        qu_code_print(ctx,
            "if(${expr} > ${max:d})\n"
            "    qu_report_error(ctx, node${level:d}, "
                "`Integer too big, max ${max:d}`);\n"
            , "max:l", self->max
            , "level:d", level
            , "expr", expr
            , NULL);
    }

}

static void qu_int_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname)
{
    (void) opt;
    qu_code_print(ctx,
        "long ${varname:c};\n",
        "varname", varname,
        NULL);
}

static void qu_int_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag)
{
    (void) opt;
    qu_code_print(ctx,
        "char buf[24];\n"
        "int vlen = sprintf(buf, `%ld`, ${expr});\n"
        "qu_emit_scalar(ctx, ${tag}, NULL, 0, buf, vlen);\n"
        , "tag", tag
        , "expr", expr
        , NULL);
}

static void qu_int_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    struct qu_int_option *self = opt->typedata;
    long defvalue = self->defvalue;
    if(!self->defvalue_set) {
        /*  Reset value to zero to be on the safe side  */
        defvalue = 0;
    }
    qu_code_print(ctx,
        "${expr} = ${defvalue:l};\n",
        "expr", expr,
        "defvalue:l", defvalue,
        NULL);
}
