#include <ctype.h>
#include <assert.h>

#include "cli.h"
#include "context.h"
#include "util/print.h"
#include "../util/wrap.h"
#include "types/types.h"
#include "../yaml/access.h"

struct qu_cli_group {
    const char *name;
    TAILQ_ENTRY(qu_cli_grp_lst) lst;
    TAILQ_HEAD(qu_cli_options_lst, qu_cli_optref) children;
};

struct qu_cli_optref {
    const char *name;
    const char *action;
    const char *metavar;
    const char *group;
    const char *description;
    struct qu_option *opt;

    /*  List by group in definition order  */
    TAILQ_ENTRY(qu_cli_optref) lst;
    /*  Tree by option name  */
    struct qu_cli_optref *left;
    struct qu_cli_optref *right;
};

static void qu_help_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname);
struct qu_cli_action *qu_help_action(struct qu_option *opt, const char *action);
static void qu_config_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname);
struct qu_cli_action *qu_config_action(struct qu_option *opt, const char *action);
static void qu_print_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname);
struct qu_cli_action *qu_print_action(struct qu_option *opt, const char *action);
static void qu_define_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname);
struct qu_cli_action *qu_define_action(struct qu_option *opt, const char *action);
static void qu_check_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname);
struct qu_cli_action *qu_check_action(struct qu_option *opt, const char *action);

static struct qu_option_vptr qu_cli_help_vptr = {
    /* parse */ NULL,
    /* cli_action */ qu_help_action,
    /* cli_parser */ qu_help_parser,
    /* parser */ NULL,
    /* definition */ NULL,
    /* printer */ NULL,
    /* default_setter */ NULL
    };
static struct qu_option_vptr qu_cli_config_vptr = {
    /* parse */ NULL,
    /* cli_action */ qu_config_action,
    /* cli_parser */ qu_config_parser,
    /* parser */ NULL,
    /* definition */ NULL,
    /* printer */ NULL,
    /* default_setter */ NULL
    };
static struct qu_option_vptr qu_cli_define_vptr = {
    /* parse */ NULL,
    /* cli_action */ qu_define_action,
    /* cli_parser */ qu_define_parser,
    /* parser */ NULL,
    /* definition */ NULL,
    /* printer */ NULL,
    /* default_setter */ NULL
    };
static struct qu_option_vptr qu_cli_check_vptr = {
    /* parse */ NULL,
    /* cli_action */ qu_check_action,
    /* cli_parser */ qu_check_parser,
    /* parser */ NULL,
    /* definition */ NULL,
    /* printer */ NULL,
    /* default_setter */ NULL
    };
static struct qu_option_vptr qu_cli_print_vptr = {
    /* parse */ NULL,
    /* cli_action */ qu_print_action,
    /* cli_parser */ qu_print_parser,
    /* parser */ NULL,
    /* definition */ NULL,
    /* printer */ NULL,
    /* default_setter */ NULL
    };

void qu_cli_init(struct qu_context *ctx) {
    ctx->cli_options.option_index = NULL;
    TAILQ_INIT(&ctx->cli_options.groups);
}

void qu_cli_print_struct(struct qu_context *ctx) {
    qu_code_print(ctx,
        "struct ${pref}_cli {\n"
        "    const char *cfg_filename;\n"
        "    int action;\n"
        "    uint32_t print_flags;\n"
        , NULL);
    qu_code_print(ctx,
        "};\n"
        "\n",
        NULL);
}

void qu_cli_parser_visit_long(struct qu_context *ctx,
                              struct qu_cli_optref *opt)
{
    if(!opt)
        return;
    qu_cli_parser_visit_long(ctx, opt->left);

    if(opt->name[1] == '-') {
        qu_code_print(ctx,
            "if(!strncmp(*arg, ${optname:q}, ${optlen:d}) && "
            "((*arg)[${optlen:d}] == 0 || (*arg)[${optlen:d}] == '=')) {\n",
            "optname", opt->name,
            "optlen:d", strlen(opt->name),
            NULL);
        struct qu_cli_action *act;
        act = opt->opt->vp->cli_action(opt->opt, opt->action);
        if(act->has_arg) {
            qu_code_print(ctx,
                "const char  *value;\n"
                "if((*arg)[${optlen:d}] == '=') {\n"
                "   value = *arg + ${optlen:d} + 1;\n"
                "} else {\n"
                "    value = *(++arg);\n"
                "    argc -= 1;\n"
                "    if(!argc)\n"
                "        qu_report_error(ctx, NULL, "
                        "`Option ${optname} needs an argument`);\n"
                "}\n",
                "optname", opt->name,
                "optlen:d", strlen(opt->name),
                NULL);
            opt->opt->vp->cli_parser(ctx, opt->opt, opt->action, "value");
        } else {
            qu_code_print(ctx,
                "if((*arg)[${optlen:d}] == '=')\n"
                "    qu_report_error(ctx, NULL, "
                        "`Option ${optname} doesn't accept an argument`);\n",
                "optname", opt->name,
                "optlen:d", strlen(opt->name),
                NULL);
            opt->opt->vp->cli_parser(ctx, opt->opt, opt->action, NULL);
        }
        qu_code_print(ctx,
            "    continue;\n"
            "}\n"
            , NULL);
    }

    qu_cli_parser_visit_long(ctx, opt->right);
}

void qu_cli_parser_visit_short(struct qu_context *ctx,
                               struct qu_cli_optref *opt)
{
    if(!opt)
        return;
    qu_cli_parser_visit_short(ctx, opt->left);

    if(opt->name[1] != '-') {
        qu_code_print(ctx,
            "case '${char}':\n",
            "char", opt->name+1,
            NULL);
        struct qu_cli_action *act;
        act = opt->opt->vp->cli_action(opt->opt, opt->action);
        if(act->has_arg) {
            qu_code_print(ctx,
                "if(!*sarg) {\n"
                "    sarg = *(++arg);\n"
                "    arg += 1;\n"
                "    argc -= 2;\n"
                "    if(argc < 0)\n"
                "        qu_report_error(ctx, NULL, "
                        "`Option -${char} needs an argument`);\n"
                "}\n",
                "char", opt->name+1,
                NULL);
            opt->opt->vp->cli_parser(ctx, opt->opt, opt->action, "sarg");
            qu_code_print(ctx,
                "goto nextarg;\n"
                , NULL);
        } else {
            opt->opt->vp->cli_parser(ctx, opt->opt, opt->action, NULL);
            qu_code_print(ctx,
                "break;\n"
                , NULL);
        }
    }

    qu_cli_parser_visit_short(ctx, opt->right);
}

void qu_cli_print_parser(struct qu_context *ctx) {
    qu_code_print(ctx,
        "void ${pref}_cli_parse(struct qu_config_context *ctx, "
            "struct ${pref}_cli *cli, int argc, char **argv) {\n"
        "    char **arg;\n"
        "    char *sarg;\n"
        "    cli->action = QU_CLI_RUN;\n"
        "    cli->print_flags = 0;\n"
        "    cli->cfg_filename = ${defaultfn:q};\n"
        "    for(arg = argv; ; ++arg, --argc) {\n"
        "    nextarg:\n"
        "       if(!argc) break;\n"
        "       if((*arg)[0] != '-')\n"
        "           break;\n"  // TODO(tailhook) check if supported
        "       if((*arg)[1] == '-') {  /*  Long arguments */\n"
        , "defaultfn", ctx->meta.default_config
        , NULL);


    qu_cli_parser_visit_long(ctx, ctx->cli_options.option_index);

    qu_code_print(ctx,
        "       } else {  /*  Short arguments */\n"
        "           for(sarg = *arg+1; *sarg;) {\n"
        "               switch(*sarg++) {\n"
        , NULL);

    qu_cli_parser_visit_short(ctx, ctx->cli_options.option_index);


    qu_code_print(ctx,
        "                default:\n"
        "                    qu_report_error(ctx, NULL, `Unknown option`);\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n"
        "\n"
        , NULL);
}

void qu_cli_print_applier(struct qu_context *ctx) {
    qu_code_print(ctx,
        "void ${pref}_cli_apply(struct ${pref}_main *cfg, "
                             "struct ${pref}_cli *cli) {\n"
        , NULL);


    qu_code_print(ctx,
        "}\n"
        "\n"
        , NULL);
}

void qu_cli_print_fwdecl(struct qu_context *ctx) {
    qu_code_print(ctx,
        "void ${pref}_cli_parse(struct qu_config_context *ctx, "
            "struct ${pref}_cli *cli, int argc, char **argv);\n"
        "void ${pref}_cli_apply(struct ${pref}_main *cfg, "
                             "struct ${pref}_cli *cli);\n"
        , NULL);
}

static struct qu_cli_optref **qu_cli_find(struct qu_context *ctx,
    const char *name)
{
    struct qu_cli_optref **node = &ctx->cli_options.option_index;
    while(*node) {
        int rc = strcmp((*node)->name, name);
        if(rc == 0)
            return node;
        if(rc < 0)
            node = &(*node)->left;
        if(rc > 0)
            node = &(*node)->right;
    }
    return node;
}

static struct qu_cli_group *qu_cli_new_group(struct qu_context *ctx,
    const char *gname)
{
    struct qu_cli_group *grp = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_cli_group));
    TAILQ_INIT(&grp->children);
    grp->name = gname;
    return grp;
}

int qu_cli_add(struct qu_context *ctx, const char *opt, const char *action,
    const char *metavar, struct qu_option *option,
    const char *group, const char *descr)
{
    assert (opt && *opt);
    struct qu_cli_optref **place = qu_cli_find(ctx, opt);
    if(*place)
        return 0;
    struct qu_cli_optref *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_cli_optref));
    self->name = opt;
    self->action = action;
    self->metavar = metavar;
    self->group = group;
    self->description = descr;
    self->opt = option;

    struct qu_cli_group *grp;
    TAILQ_FOREACH(grp, &ctx->cli_options.groups, lst) {
        if(!strcmp(grp->name, group)) {
            break;
        }
    }
    if(!grp) {
        grp = qu_cli_new_group(ctx, group);
        TAILQ_INSERT_TAIL(&ctx->cli_options.groups, grp, lst);
    }
    TAILQ_INSERT_TAIL(&grp->children, self, lst);

    self->left = NULL;
    self->right = NULL;
    *place = self;

    return 1;
}

void qu_cli_add_quire(struct qu_context *ctx) {
    struct qu_option *help = qu_option_new(ctx, &qu_cli_help_vptr);
    const char *group = "Configuration Options";
    qu_cli_add(ctx, "-h", NULL, NULL, help,
        group, "Print this help");
    qu_cli_add(ctx, "--help", NULL, NULL, help,
        group, "Print this help");

    const char *config_descr = qu_template_alloc(ctx,
        "Configuration file name [default: ${filename}]",
        "filename", ctx->meta.default_config,
        NULL);
    struct qu_option *config = qu_option_new(ctx, &qu_cli_config_vptr);
    qu_cli_add(ctx, "-c", NULL, "PATH", config,
        group, config_descr);
    qu_cli_add(ctx, "--config", NULL, "PATH", config,
        group, config_descr);

    struct qu_option *define = qu_option_new(ctx, &qu_cli_define_vptr);
    qu_cli_add(ctx, "-D", "update", "NAME=VALUE", define,
        group, "Set value of configuration variable NAME to VALUE");
    qu_cli_add(ctx, "--config-var", "update", "NAME=VALUE", define,
        group, "Set value of configuration variable NAME to VALUE");

    struct qu_option *check = qu_option_new(ctx, &qu_cli_check_vptr);
    qu_cli_add(ctx, "-C", NULL, NULL, check,
        group, "Check configuration and exit");
    qu_cli_add(ctx, "--config-check", NULL, NULL, check,
        group, "Check configuration and exit");

    struct qu_option *print = qu_option_new(ctx, &qu_cli_print_vptr);
    qu_cli_add(ctx, "-P", "incr", NULL, print,
        group, "Print configuration after reading, then exit. "
               "The configuration printed by this option includes values "
               "overriden from command-line. Double flag `-PP` prints "
               "comments.");
    qu_cli_add(ctx, "--config-print", NULL, "TYPE", print,
        group, "Print configuration file after reading. TYPE maybe "
               "\"current\", \"details\", \"example\", \"all\", \"full\"");
}

static int qu_cmp(const char *a, const char *b) {
    return (a == b || (a && b && !strcmp(a, b)));
}

const char *qu_cli_format_usage(struct qu_context *ctx) {
    const char *ptr, *end;
    struct obstack *buf = &ctx->parser.pieces;
    obstack_blank(buf, 0);
    qu_template_grow(ctx,
        "Usage:\n"
        "    ${prog} [-c CONFIG_PATH] [options]\n"
        "\n",
        "prog", ctx->meta.program_name,
        "fn", ctx->meta.default_config,
        NULL);
    ptr = ctx->meta.description;
    end = ptr + strlen(ptr);
    while(ptr < end) {
        ptr = qu_line_grow(buf, ptr, end, 80);
        obstack_1grow(buf, '\n');
    }

    struct qu_cli_group *grp;
    TAILQ_FOREACH(grp, &ctx->cli_options.groups, lst) {
        qu_template_grow(ctx,
            "\n${groupname}:\n",
            "groupname", grp->name,
            NULL);
        struct qu_cli_optref *opt, *nxt;
        for(opt = TAILQ_FIRST(&grp->children); opt; opt = nxt) {
            int startoff = obstack_object_size(buf);
            qu_template_grow(ctx,
                "  ${opt}",
                "opt", opt->name,
                NULL);
            nxt = TAILQ_NEXT(opt, lst);
            while(nxt && nxt->opt == opt->opt
               && qu_cmp(nxt->action, opt->action)
               && qu_cmp(nxt->metavar, opt->metavar)
               && qu_cmp(nxt->group, opt->group)
               && qu_cmp(nxt->description, opt->description)) {
                qu_template_grow(ctx,
                    ",${opt}",
                    "opt", nxt->name,
                    NULL);
                nxt = TAILQ_NEXT(nxt, lst);
            }
            if(opt->metavar)
                qu_template_grow(ctx,
                    " ${var}",
                    "var", opt->metavar,
                    NULL);
            int endoff = obstack_object_size(buf);
            if(endoff - startoff > 18) {
                obstack_grow(buf, "\n                    ", 21);
            } else {
                obstack_grow(buf,
                    "                    ", 20 - (endoff - startoff));
            }
            ptr = opt->description;
            end = ptr + strlen(ptr);
            ptr = qu_line_grow(buf, ptr, end, 60);
            while(ptr < end) {
                obstack_grow(buf, "\n                    ", 21);
                ptr = qu_line_grow(buf, ptr, end, 60);
            }
            obstack_1grow(buf, '\n');
        }
    }
    obstack_1grow(buf, 0);

    return obstack_finish(buf);
}

static struct qu_cli_optref *qu_cli_parse_ref(struct qu_context *ctx,
    struct qu_option *opt, const char *action, struct qu_cli_action *act,
    qu_ast_node *node, qu_ast_node *defnode)
{
    qu_ast_node *tmp;
    const char *descr = NULL;
    const char *group = "Options";
    const char *metavar = act->metavar;
    qu_ast_node *name = NULL;
    qu_ast_node *names = NULL;

    if((tmp = qu_map_get(defnode, "group")))
        group = qu_node_content(tmp);

    struct qu_cli_optref *ref = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_cli_optref));

    if(node->kind == QU_NODE_MAPPING) {
        name = qu_map_get(node, "name");
        names = qu_map_get(node, "names");
        if((tmp = qu_map_get(node, "="))) {
            if(tmp->kind == QU_NODE_SEQUENCE)
                names = tmp;
            else
                name = tmp;
        }
        if((tmp = qu_map_get(node, "metavar")))
            metavar = qu_node_content(tmp);
        if((tmp = qu_map_get(node, "descr")))
            descr = qu_node_content(tmp);
        if((tmp = qu_map_get(node, "group")))
            group = qu_node_content(tmp);
    } else if(node->kind == QU_NODE_SCALAR) {
        name = node;
    } else if(node->kind == QU_NODE_SEQUENCE) {
        names = node;
    }
    if(!descr && !action) {
        descr = opt->description;
    }
    if(!descr) {
        descr = qu_template_alloc(ctx, act->descr,
            "name", opt->path,
            NULL);
    }
    if(name) {
        const char *oname = qu_node_content(name);
        qu_cli_add(ctx, oname, action, metavar, opt, group, descr);
    }
    if(names) {
        qu_seq_member *item;
        TAILQ_FOREACH(item, &names->val.seq_index.items, lst) {
            const char *oname = qu_node_content(item->value);
            qu_cli_add(ctx, oname, action, metavar, opt, group, descr);
        }
    }

    return ref;
}

void qu_cli_parse(struct qu_context *ctx,
	struct qu_option *opt, qu_ast_node *node)
{
    const int clen = strlen("command-line");
    qu_ast_node *clinode = qu_map_get(node, "command-line");
    struct qu_cli_action *act = NULL;

    if(clinode) {
        act = opt->vp->cli_action(opt, NULL);
        if(!act) {
            LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                clinode->start_token,
                "Unsupported command-line action");
        }
        qu_cli_parse_ref(ctx, opt, NULL, act, clinode, clinode);
    }

    qu_map_member *item;
    TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
        const char *mname = qu_node_content(item->key);
        if(strncmp(mname, "command-line-", clen+1)) {
            continue;
        }
        const char *action = mname + clen + 1;
        act = opt->vp->cli_action(opt, action);
        if(!act) {
            LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser,
                item->key->start_token,
                "Unsupported command-line action");
        }
        qu_cli_parse_ref(ctx, opt, action, act, item->value, clinode);
    }
}
struct qu_cli_action *qu_help_action(struct qu_option *opt, const char *action)
{
    static struct qu_cli_action set = {0, "Print this help", NULL};
    return &set;
}
static void qu_help_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    qu_code_print(ctx,
        "cli->action = QU_CLI_PRINT_HELP;\n"
        , NULL);
}
struct qu_cli_action *qu_config_action(struct qu_option *opt, const char *action)
{
    static struct qu_cli_action set = {1, "Parse file PATH", "PATH"};
    return &set;
}
static void qu_config_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    qu_code_print(ctx,
        "cli->cfg_filename = ${argname};\n",
        "argname", argname,
        NULL);
}
struct qu_cli_action *qu_print_action(struct qu_option *opt, const char *action)
{
    static struct qu_cli_action set = {1, "Print config CONFIG", "CONFIG"};
    static struct qu_cli_action incr = {0, "Print config", NULL};
    if(action == NULL)
        return &set;
    return &incr;
}
static void qu_print_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    qu_code_print(ctx,
        "cli->action = QU_CLI_PRINT_CONFIG;\n"
        , NULL);
    if(action == NULL) {
        qu_code_print(ctx,
            "if(!strcmp(${argname}, `current`))\n"
            "    cli->print_flags = 0;\n"
            "if(!strcmp(${argname}, `example`))\n"
            "    cli->print_flags = QU_PRINT_EXAMPLE|QU_PRINT_COMMENTS;\n"
            "if(!strcmp(${argname}, `details`))\n"
            "    cli->print_flags = QU_PRINT_COMMENTS;\n"
            "if(!strcmp(${argname}, `all`))\n"
            "    cli->print_flags = QU_PRINT_FULL;\n"
            "if(!strcmp(${argname}, `full`))\n"
            "    cli->print_flags = QU_PRINT_FULL|QU_PRINT_COMMENTS;\n",
            "argname", argname,
            NULL);
    }
}
struct qu_cli_action *qu_define_action(struct qu_option *opt, const char *action)
{
    static struct qu_cli_action set = {1, "Define NAME=VALUE", "NAME=VALUE"};
    return &set;
}
static void qu_define_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    qu_code_print(ctx,
        "// TODO: define\n",
        NULL);
}
struct qu_cli_action *qu_check_action(struct qu_option *opt, const char *action)
{
    static struct qu_cli_action set = {0, "Check config and exit", NULL};
    return &set;
}
static void qu_check_parser(struct qu_context *ctx,
    struct qu_option *opt, const char *action, const char *argname)
{
    qu_code_print(ctx,
        "cli->action = QU_CLI_CHECK_CONFIG;\n",
        NULL);
}
