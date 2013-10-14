#ifndef QUIRE_H_CFG_API
#define QUIRE_H_CFG_API

#include "../yaml/node.h"

struct qu_config_context *qu_config_parser();
void qu_config_parser_free(struct qu_config_context *ctx);
qu_ast_node *qu_config_parse_yaml(struct qu_config_context *ctx,
    const char *filename);

#endif  // QUIRE_H_CFG_API
