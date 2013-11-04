#ifndef QUIRE_H_CFG_EVAL
#define QUIRE_H_CFG_EVAL

#include "../yaml/node.h"

struct qu_config_context;

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
void qu_node_to_int(struct qu_config_context *ctx, qu_ast_node *node,
    long *result);
void qu_node_to_bool(struct qu_config_context *ctx, qu_ast_node *node,
    int *result);
void qu_node_to_float(struct qu_config_context *ctx, qu_ast_node *node,
    double *result);
void qu_node_to_str(struct qu_config_context *ctx, qu_ast_node *node,
    const char **result, int *rlen);

#endif // QUIRE_H_CFG_EVAL
