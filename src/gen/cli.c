#include "cli.h"
#include "context.h"
#include "util/print.h"

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
    qu_code_print(ctx,
        "struct `pref`_cli {\n"
        , NULL);
    qu_code_print(ctx,
        "};\n"
        "\n",
        NULL);
}
