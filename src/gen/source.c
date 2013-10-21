#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "source.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"
#include "util/name.h"
#include "util/print.h"
#include "context.h"
#include "struct.h"
#include "classes/classes.h"
#include "../quire_int.h"

int qu_output_source(struct qu_context *ctx) {
    qu_code_print(ctx,
        "/*  DO NOT EDIT THIS FILE!  */\n"
        "/*  This file is generated  */\n"
        "#include <stdio.h>\n"
        "#include <errno.h>\n"
        "#include <stdlib.h>\n"
        "#include <getopt.h>\n"
        "#include <string.h>\n"
        "#include <stdint.h>\n"
        "\n"
        "#include \"config.h\"\n"  // TODO(tailhook) fix config name
        "\n"
        , NULL);

    qu_classes_print_functions(ctx);
    qu_cli_print_parser(ctx);
    qu_cli_print_applier(ctx);

    qu_code_print(ctx,
        "int ${pref}_set_defaults(struct ${pref}_main *cfg) {\n"
        , NULL);

    if(ctx->root) {
        qu_struct_default_setter(ctx, ctx->root, "cfg->");
    }

    qu_code_print(ctx,
        "return 0;\n"
        "}\n"
        "\n"
        , NULL);

    qu_code_print(ctx,
        "int ${pref}_parse(struct qu_config_context *ctx, "
            "struct ${pref}_cli *cli, struct ${pref}_main *cfg) {\n"
        "qu_ast_node *node0 = qu_config_parse_yaml(ctx, cli->cfg_filename);\n"
        , NULL);

    if(ctx->root) {
        qu_struct_parser(ctx, ctx->root, "cfg->", 0);
    }

    qu_code_print(ctx,
        "return 0;\n"
        "}\n"
        "\n"
        , NULL);


    qu_code_print(ctx,
        "int ${pref}_load(struct ${pref}_main *cfg, int argc, char **argv) {\n"
        "int rc;\n"
        "jmp_buf jmp;\n"
        "struct qu_config_context *ctx = NULL;\n"
        "struct ${pref}_cli cli;\n"
        "\n"
        "qu_config_init(&cfg->head, sizeof(*cfg));\n"
        "${pref}_set_defaults(cfg);\n"
        "if(!(rc = setjmp(jmp))) {\n"
        "    ctx = qu_config_parser(&cfg->head, &jmp);\n"
        "    ${pref}_cli_parse(ctx, &cli, argc-1, argv+1);\n"
        "    switch(cli.action) {\n"
        "        case QU_CLI_PRINT_HELP:\n"
        "            break;  /*  Do not parse config  */\n"
        "        case QU_CLI_PRINT_CONFIG:\n"
        "            if(cli.print_flags & QU_PRINT_EXAMPLE)\n"
        "                break;  /*  Do not parse config  */\n"
        "        default:\n"
        "            ${pref}_parse(ctx, &cli, cfg);\n"
        "            //${pref}_cli_apply(ctx, &cliv);\n"
        "    }\n"
        "    switch(cli.action) {\n"
        "        case QU_CLI_RUN:\n"
        "           break;\n"
        "        case QU_CLI_CHECK_CONFIG:\n"
        "           /*  Do nothing, config checking is a side-effect anyway  */\n"
        "           rc = -1;\n"
        "           break;\n"
        "        case QU_CLI_PRINT_CONFIG:\n"
        "           ${pref}_print(cfg, cli.print_flags, stdout);\n"
        "           rc = -1;\n"
        "           break;\n"
        "        case QU_CLI_PRINT_HELP:\n"
        "           ${pref}_help(stdout);\n"
        "           rc = -1;\n"
        "           break;\n"
        "    }"
        "}  /*  end of setjmp */\n"
        // TODO(tailhook) print errors
        "if(ctx) {\n"
        "    qu_config_parser_free(ctx);\n"
        "    if(rc > 0)\n"
        "        qu_print_error(ctx, stderr);\n"
        "}\n"
        "return rc;\n"
        "}\n"
        "\n"
        , NULL);

    qu_code_print(ctx,
        "void ${pref}_free(struct ${pref}_main *cfg) {\n"
        "    qu_config_free(cfg);\n"
        "}\n"
        "\n"
        , NULL);


    qu_code_print(ctx,
        "void ${pref}_help(FILE *stream) {\n"
        "   fprintf(stream, ${usage:q});\n"
        "}\n"
        "\n",
        "usage", qu_cli_format_usage(ctx),
        NULL);

    int underlen = strlen(ctx->meta.program_name);
    char underline[underlen+1];
    memset(underline, '=', underlen);
    underline[underlen] = 0;

    qu_code_print(ctx,
        "void ${pref}_print(struct ${pref}_main *cfg, int flags, FILE *stream) {\n"
        "qu_emit_context cctx;\n"
        "qu_emit_context *ctx = &cctx;\n"
        "qu_emit_init(ctx, stream);\n"
        "\n"
        "if(flags & QU_PRINT_COMMENTS) {\n"
        "qu_emit_comment(ctx, 0, ${name:q}, -1);\n"
        "qu_emit_comment(ctx, 0, ${underline:q}, -1);\n"
        "qu_emit_comment(ctx, 0, ``, 0);\n"
        "qu_emit_comment(ctx, 0, ${description:q}, -1);\n"
        "qu_emit_comment(ctx, 0, ``, 0);\n"
        "qu_emit_whitespace(ctx, QU_WS_ENDLINE, 1);\n"
        "}\n"
        "\n",
        "name", ctx->meta.program_name,
        "description", ctx->meta.description,
        "underline", underline,
        NULL);

    if(ctx->root) {
        qu_struct_printer(ctx, ctx->root, "cfg->", "NULL");
    }

    qu_code_print(ctx,
        "qu_emit_done(ctx);\n"
        "}\n"
        "\n"
        , NULL);

    return 0;
}
