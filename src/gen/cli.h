#ifndef QUIRE_H_GEN_CLI
#define QUIRE_H_GEN_CLI

#include <sys/queue.h>
#include "../yaml/node.h"

struct qu_context;
struct qu_option;
struct qu_guard;

struct qu_cli_options {
    TAILQ_HEAD(qu_cli_grp_lst, qu_cli_group) groups;
    TAILQ_HEAD(qu_cli_plain_lst, qu_cli_optref) all;

    struct qu_cli_optref *option_index;
};

struct qu_cli_action {
    int has_arg;
    const char *descr;
    const char *metavar;
};

void qu_cli_init(struct qu_context *ctx);
void qu_cli_print_struct(struct qu_context *ctx);
void qu_cli_print_parser(struct qu_context *ctx);
void qu_cli_print_applier(struct qu_context *ctx);
void qu_cli_print_fwdecl(struct qu_context *ctx);
void qu_cli_add_quire(struct qu_context *ctx);
void qu_cli_parse(struct qu_context *ctx,
    struct qu_option *opt, qu_ast_node *node, struct qu_guard *g);

void qu_cli_print_usage(struct qu_context *ctx);

#endif  // QUIRE_H_GEN_CLI
