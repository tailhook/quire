#ifndef QUIRE_H_CFG_EVAL
#define QUIRE_H_CFG_EVAL

struct qu_parser;
struct qu_var_frame;

#include "../yaml/node.h"

void qu_eval_str(struct qu_parser *parser, struct qu_var_frame *vars,
    const char *data, qu_ast_node *node,
    const char **result, int *rlen);

#endif // QUIRE_H_CFG_EVAL
