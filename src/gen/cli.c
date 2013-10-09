#include "cli.h"
#include "context.h"

struct qu_cli_group {
    char *name;
};

struct qu_cli_option {
    char shortoption;
    int longoptions_len;
    char **longoptions;
    struct qu_option *option;
};

void qu_cli_print_struct(struct qu_context *ctx) {

}
