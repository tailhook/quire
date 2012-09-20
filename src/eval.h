#ifndef _H_EVAL
#define _H_EVAL

#include <stdint.h>

#include "yparser.h"
#include "context.h"

void _qu_eval_int(qu_parse_context *info,
    char *value, int interp, long *result);
void _qu_eval_float(qu_parse_context *info,
    char *value, int interp, double *result);
void _qu_eval_str(qu_parse_context *info,
    char *value, int interp, char **result, size_t *rlen);

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
#define QU_FLAGS_VARS       1

void qu_node_to_int(qu_parse_context *ctx, qu_ast_node *node, uint64_t flags,
    long *result);
void qu_node_to_float(qu_parse_context *ctx, qu_ast_node *node, uint64_t flags,
    double *result);
void qu_node_to_str(qu_parse_context *ctx, qu_ast_node *node, uint64_t flags,
    char **result, size_t *rlen);

#endif // _H_EVAL
