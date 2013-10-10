#ifndef QUIRE_H_GEN_CLI
#define QUIRE_H_GEN_CLI

#include <sys/queue.h>

struct qu_context;

struct qu_cli_options {
    TAILQ_HEAD(qu_cli_grp_lst, lst) groups;

    struct qu_cli_optref *option_index;
};

void qu_cli_init(struct qu_context *ctx);
void qu_cli_print_struct(struct qu_context *ctx);
void qu_cli_print_parser(struct qu_context *ctx);
void qu_cli_print_applier(struct qu_context *ctx);
void qu_cli_print_fwdecl(struct qu_context *ctx);
void qu_cli_add_quire(struct qu_context *ctx);

const char *qu_cli_format_usage(struct qu_context *ctx);

#endif  // QUIRE_H_GEN_CLI
