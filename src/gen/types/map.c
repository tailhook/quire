#include "map.h"
#include "types.h"
#include "../process.h"
#include "../struct.h"
#include "../cli.h"
#include "../context.h"
#include "../../util/parse.h"
#include "../../yaml/access.h"
#include "../../quire_int.h"
#include "../util/print.h"

static void qu_map_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node);
static void qu_map_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level);
static void qu_map_definition(struct qu_context *ctx,
    struct qu_option *opt, const char *varname);
static void qu_map_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag);
static void qu_map_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr);

struct qu_option_vptr qu_map_vptr = {
    /* parse */ qu_map_parse,
    /* cli_action */ NULL,
    /* cli_parser */ NULL,
    /* cli_definition */ NULL,
    /* cli_apply */ NULL,
    /* parse */ qu_map_parser,
    /* definition */ qu_map_definition,
    /* printer */ qu_map_printer,
    /* default_setter */ qu_map_default_setter
};

static int mapping_index = 0;

struct qu_map_option {
    char *typename;
    struct qu_option *key;
    int is_struct;
    int idx;
    union {
        struct qu_config_struct *str;
        struct qu_option *opt;
    } val;
};

static void qu_map_decl(struct qu_context *ctx, struct qu_option *opt) {
    qu_code_print(ctx,
        "struct ${pref}_${typname} {\n"
        "    struct ${pref}_${typname} *next;\n"
        , "typname", opt->typname
        , NULL);
    struct qu_map_option *self = opt->typedata;
    self->key->vp->definition(ctx, self->key, "key");
    if(self->is_struct) {
        qu_struct_definition(ctx, self->val.str);
    } else {
        self->val.opt->vp->definition(ctx, self->val.opt, "val");
    }
    qu_code_print(ctx, "};\n", NULL);
}

static void qu_map_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node)
{
    struct qu_map_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_map_option));
    opt->typedata = self;
    self->idx = mapping_index++;

    if(node->kind != QU_NODE_MAPPING) {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
            "Mapping type definition must be mapping");
    }

    qu_ast_node *key, *val;
    key = qu_map_get(node, "key-element");
    val = qu_map_get(node, "value-element");

    if(!key || !val) {
        LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, node->start_token,
            "Mapping type definition must have key-element and value-element");
    }
    self->key = qu_parse_option(ctx, key, "<key>", NULL);
    self->is_struct = !val->tag;
    if(self->is_struct) {
        if(val->kind == QU_NODE_MAPPING) {
            self->val.str = qu_struct_new_root(ctx);
            qu_visit_struct_children(ctx, val, self->val.str);
        } else {
            LONGJUMP_ERR_NODE(ctx, val, "Untagged straw scalar");
        }
        opt->typname = qu_template_alloc(ctx, "m_${key}_${optpath:c}"
            , "key", self->key->typname
            , "optpath", opt->path
            , NULL);
    } else {
        self->val.opt = qu_parse_option(ctx, val, "[]", NULL);
        opt->typname = qu_template_alloc(ctx, "m_${key}_${typname}"
            , "key", self->key->typname
            , "typname", self->val.opt->typname
            , NULL);
    }

    const char *sname = qu_template_alloc(ctx, "struct ${pref}_${typname}",
        "typname", opt->typname,
        NULL);

    qu_fwdecl_add(ctx, sname, (qu_fwdecl_printer)qu_map_decl, opt);
}

static void qu_map_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, int level)
{
    struct qu_map_option *self = opt->typedata;
    qu_code_print(ctx,
        "struct qu_map_member *mem;\n"
        "for(mem = qu_map_iter(node${level:d}); mem; mem = qu_map_next(mem)) {\n"
        "   qu_ast_node *node${nlevel:d};\n"
        "   node${nlevel:d}= qu_map_key(mem);\n"
        "   struct ${pref}_${typname} *el${level:d} = "
            "qu_config_alloc(ctx, sizeof(struct ${pref}_${typname}));\n"
        , "level:d", level
        , "nlevel:d", level+1
        , "typname", opt->typname
        , NULL);

    const char *elkey = qu_template_alloc(ctx,
        "el${level:d}->key", "level:d", level, NULL);
    self->key->vp->default_setter(ctx, self->key, elkey);
    self->key->vp->parser(ctx, self->key, elkey, level+1);

    qu_code_print(ctx,
        "   node${nlevel:d}= qu_map_value(mem);\n"
        , "nlevel:d", level+1
        , NULL);

    if(self->is_struct) {
        const char *elpref = qu_template_alloc(ctx,
            "el${level:d}->", "level:d", level, NULL);
        qu_struct_default_setter(ctx, self->val.str, elpref);
        qu_struct_parser(ctx, self->val.str, elpref, level+1);
    } else {
        const char *elname = qu_template_alloc(ctx,
            "el${level:d}->val", "level:d", level, NULL);
        self->val.opt->vp->default_setter(ctx, self->val.opt, elname);
        self->val.opt->vp->parser(ctx, self->val.opt, elname, level+1);
    }

    qu_code_print(ctx,
        "*${expr}_tail = el${level:d};\n"
        "${expr}_tail = &el${level:d}->next;\n"
        "${expr}_len += 1;\n"
        "}\n"
        , "expr", expr
        , "level:d", level
        , NULL);
}

static void qu_map_definition(struct qu_context *ctx,
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

static void qu_map_printer(struct qu_context *ctx,
    struct qu_option *opt, const char *expr, const char *tag)
{
    struct qu_map_option *self = opt->typedata;
    qu_code_print(ctx,
        "qu_emit_opcode(ctx, ${tag}, NULL, QU_EMIT_MAP_START);\n"
        "struct ${pref}_${typname} *el${idx:d};\n"
        "for(el${idx:d} = ${expr}; el${idx:d}; el${idx:d} = el${idx:d}->next) {\n"
        , "tag", tag
        , "typname", opt->typname
        , "idx:d", self->idx
        , "expr", expr
        , NULL);

    const char *elkey = qu_template_alloc(ctx,
        "el${idx:d}->key", "idx:d", self->idx, NULL);
    self->key->vp->printer(ctx, self->key, elkey, "NULL");

    qu_code_print(ctx,
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n"
        , NULL);

    if(self->is_struct) {
        const char *elpref = qu_template_alloc(ctx,
            "el${idx:d}->", "idx:d", self->idx, NULL);
        qu_struct_printer(ctx, self->val.str, elpref, "NULL");
    } else {
        const char *elname = qu_template_alloc(ctx,
            "el${idx:d}->val", "idx:d", self->idx, NULL);
        self->val.opt->vp->printer(ctx, self->val.opt, elname, "NULL");
    }

    qu_code_print(ctx,
        "}\n"
        "qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n"
        , NULL);
}

static void qu_map_default_setter(struct qu_context *ctx,
    struct qu_option *opt, const char *expr)
{
    (void) opt;
    qu_code_print(ctx,
        "${expr} = NULL;\n"
        "${expr}_tail = &${expr};\n"
        "${expr}_len = 0;\n",
        "expr", expr,
        NULL);
}
