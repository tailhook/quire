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
