#ifndef QUIRE_H_GEN_CLI
#define QUIRE_H_GEN_CLI

#include <sys/queue.h>

struct qu_context;

struct qu_cli_options {
    LIST_HEAD(qu_cli_grp_lst, lst) groups;

    struct qu_cli_optref *option_index;

    struct qu_cli_option *qu_cli_shopt_table[256];
};

void qu_cli_print_struct(struct qu_context *ctx);
void qu_cli_print_parser(struct qu_context *ctx);
void qu_cli_print_applier(struct qu_context *ctx);
void qu_cli_print_fwdecl(struct qu_context *ctx);

#endif  // QUIRE_H_GEN_CLI
