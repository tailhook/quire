#ifndef QUIRE_H_CFG_EVAL
#define QUIRE_H_CFG_EVAL

#include "../yaml/node.h"

struct qu_config_context;

void qu_eval_int(struct qu_config_context *ctx,
    char *value, int interp, long *result);
void qu_eval_float(struct qu_config_context *ctx,
    char *value, int interp, double *result);
void qu_eval_str(struct qu_config_context *ctx,
    char *value, int interp, char **result, size_t *rlen);

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
void qu_node_to_int(struct qu_config_context *ctx, qu_ast_node *node,
    long *result);
void qu_node_to_float(struct qu_config_context *ctx, qu_ast_node *node,
    double *result);
void qu_node_to_str(struct qu_config_context *ctx, qu_ast_node *node,
    char **result, size_t *rlen);

#endif // QUIRE_H_CFG_EVAL