#ifndef QUIRE_H_CFG_API
#define QUIRE_H_CFG_API

#include "../yaml/node.h"

struct qu_config_head;

/*  Public API  */
struct qu_config_context *qu_config_parser(
    struct qu_config_head *target, jmp_buf *jmp);
void qu_config_parser_free(struct qu_config_context *ctx);
qu_ast_node *qu_config_parse_yaml(struct qu_config_context *ctx,
    const char *filename);

void qu_report_error(struct qu_config_context *, qu_ast_node *node,
    const char *text);
void qu_cli_error(struct qu_config_context *, const char *opt,
    const char *text);
void qu_check_config_errors(struct qu_config_context *ctx);
void qu_print_config_errors(struct qu_config_context *ctx);

#endif  // QUIRE_H_CFG_API
