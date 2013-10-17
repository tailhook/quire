#include "array.h"
#include "types.h"
#include "../process.h"
#include "../struct.h"
#include "../cli.h"
#include "../context.h"
#include "../../util/parse.h"
#include "../../yaml/access.h"
#include "../../quire_int.h"
#include "../util/print.h"

static void qu_array_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node);
static void qu_array_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level);
static void qu_array_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname);
static void qu_array_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);
static void qu_array_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);

struct qu_option_vptr qu_array_vptr = {
    /* parse */ qu_array_parse,
    /* cli_action */ NULL,
    /* cli_parser */ NULL,
    /* parse */ qu_array_parser,
    /* definition */ qu_array_definition,
    /* printer */ qu_array_printer,
    /* default_setter */ qu_array_default_setter
};

struct qu_array_option {
    char *typename;
    int is_struct;
    union {
        struct qu_config_struct *str;
        struct qu_option *opt;
    } el;
};

static void qu_array_decl(struct qu_context *ctx, struct qu_option *opt) {
    qu_code_print(ctx,
        "struct ${pref}_${typname} {\n"
        "    struct ${pref}_${typname} *next;\n"
        , "typname", opt->typname
        , NULL);
    struct qu_array_option *self = opt->typedata;
    if(self->is_struct) {
        qu_struct_definition(ctx, self->el.str);
    } else {
        self->el.opt->vp->definition(ctx, self->el.opt, "val");
    }
    qu_code_print(ctx, "};\n", NULL);
}

static void qu_array_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node)
{
    struct qu_array_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_array_option));
    opt->typedata = self;

    if(node->kind != QU_NODE_MAPPING) {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
            "Array type definition must be mapping");
    }

    qu_ast_node *element;

    if((element = qu_map_get(node, "element")) ||
        (element = qu_map_get(node, "="))) {
        self->is_struct = !element->tag;
        if(self->is_struct) {
            if(element->kind == QU_NODE_MAPPING) {
                self->el.str = qu_struct_new_root(ctx);
                qu_visit_struct_children(ctx, element, self->el.str);
            } else {
                LONGJUMP_ERR_NODE(ctx, element, "Untagged straw scalar");
            }
            opt->typname = qu_template_alloc(ctx, "a_${optpath:c}",
                "optpath", opt->path,
                NULL);
        } else {
            self->el.opt = qu_parse_option(ctx, element, "[]", NULL);
            opt->typname = qu_template_alloc(ctx, "a_${typname}",
                "typname", self->el.opt->typname,
                NULL);
        }
    } else {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
            "Array type definition must contain element definition");
    }

    const char *sname = qu_template_alloc(ctx, "${pref}_${typname}",
        "typname", opt->typname,
        NULL);

    qu_fwdecl_add(ctx, sname, (qu_fwdecl_printer)qu_array_decl, opt);
}

static void qu_array_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level)
{
    struct qu_array_option *self = opt->typedata;
    qu_code_print(ctx,
        "struct qu_seq_member *mem;\n"
        "for(mem = qu_seq_iter(node${level:d}); mem; mem = qu_seq_next(mem)) {\n"
        "   qu_ast_node *node${nlevel:d} = qu_seq_node(mem);\n"
        "   struct ${pref}_${typname} *el = "
            "qu_config_alloc(cfg, sizeof(struct ${pref}_${typname}));\n"
        , "level:d", level
        , "nlevel:d", level+1
        , "typname", opt->typname
        , NULL);
    if(self->is_struct) {
        qu_struct_default_setter(ctx, self->el.str, "el->");
        qu_struct_parser(ctx, self->el.str, "el->", level+1);
    } else {
        self->el.opt->vp->default_setter(ctx, self->el.opt, "el->val");
        self->el.opt->vp->parser(ctx, self->el.opt, "el->val", level+1);
    }

    qu_code_print(ctx,
        "*${expr}_tail = el;\n"
        "${expr}_tail = &el->next;\n"
        "${expr}_len += 1;\n"
        "}\n"
        , "expr", expr
        , NULL);
}

static void qu_array_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname)
{
    qu_code_print(ctx,
        "struct ${pref}_${typname} *${varname:c};\n"
        "struct ${pref}_${typname} **${varname:c}_tail;\n"
        "int ${varname:c}_len;\n",
        "typname", opt->typname,
        "varname", varname,
        NULL);
}

static void qu_array_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    struct qu_array_option *self = opt->typedata;
    qu_code_print(ctx,
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_START);\n"
        "struct ${pref}_${typname} *el;\n"
        "for(el = ${expr}; el; el = el->next) {\n"
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_ITEM);\n"
        , "typname", opt->typname
        , "expr", expr
        , NULL);
    if(self->is_struct) {
        qu_struct_printer(ctx, self->el.str, "el->");
    } else {
        self->el.opt->vp->printer(ctx, self->el.opt, "el->val");
    }
    qu_code_print(ctx,
        "}\n"
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_END);\n"
        , NULL);
}

static void qu_array_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    qu_code_print(ctx,
        "${expr} = NULL;\n"
        "${expr}_tail = &${expr};\n"
        "${expr}_len = 0;\n",
        "expr", expr,
        NULL);
}
