#include <ctype.h>
#include <assert.h>

#include "cli.h"
#include "context.h"
#include "util/print.h"
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


static struct qu_option_vptr qu_cli_help_vptr = {
    /* parse */ NULL,
    /* cli_support */ NULL  // TODO
    };
static struct qu_option_vptr qu_cli_config_vptr = {
    /* parse */ NULL,
    /* cli_support */ NULL  // TODO
    };
static struct qu_option_vptr qu_cli_define_vptr = {
    /* parse */ NULL,
    /* cli_support */ NULL  // TODO
    };
static struct qu_option_vptr qu_cli_check_vptr = {
    /* parse */ NULL,
    /* cli_support */ NULL  // TODO
    };
static struct qu_option_vptr qu_cli_print_vptr = {
    /* parse */ NULL,
    /* cli_support */ NULL  // TODO
    };

void qu_cli_init(struct qu_context *ctx) {
    ctx->cli_options.option_index = NULL;
    TAILQ_INIT(&ctx->cli_options.groups);
}

void qu_cli_print_struct(struct qu_context *ctx) {
    qu_code_print(ctx,
        "struct ${pref}_cli {\n"
        "    int action;\n"
        "    uint32_t print_flags;\n"
        , NULL);
    qu_code_print(ctx,
        "};\n"
        "\n",
        NULL);
}

void qu_cli_print_parser(struct qu_context *ctx) {
    qu_code_print(ctx,
        "void ${pref}_cli_parse(struct qu_config_context *ctx, "
            "struct ${pref}_cli *cfg,int argc, char **argv) {\n"
        // TODO(tailhook) change to QU_CLI_RUN
        "    cfg->action = QU_CLI_PRINT_CONFIG;\n"
        "    cfg->print_flags = 0;\n"
        // TODO(tailhook) remove this crap
        "    for(char **arg = argv; *arg; ++arg) {\n"
        "        if(!strcmp(*arg, `--help`)) {\n"
        "           cfg->action = QU_CLI_PRINT_HELP;\n"
        "        }\n"
        "        if(!strcmp(*arg, `--config-print=current`)) {\n"
        "           cfg->action = QU_CLI_PRINT_CONFIG;\n"
        "           cfg->print_flags = 0;\n"
        "        }\n"
        "        if(!strcmp(*arg, `--config-print=example`)) {\n"
        "           cfg->action = QU_CLI_PRINT_CONFIG;\n"
        "           cfg->print_flags = QU_PRINT_EXAMPLE | QU_PRINT_COMMENTS;\n"
        "        }\n"
        "        if(!strcmp(*arg, `--config-print=full`)) {\n"
        "           cfg->action = QU_CLI_PRINT_CONFIG;\n"
        "           cfg->print_flags = QU_PRINT_FULL | QU_PRINT_COMMENTS;\n"
        "        }\n"
        "    }\n"
        , NULL);


    qu_code_print(ctx,
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
    printf("DESCR ``%s''\n", descr);
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
    struct qu_cli_action *act = opt->vp->cli_action(opt, NULL);

    if(clinode) {
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
        int mname_len = strlen(mname);
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
