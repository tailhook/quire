#ifndef QUIRE_H_GEN_SPECIAL
#define QUIRE_H_GEN_SPECIAL

#include "../../yaml/node.h"

struct qu_context;

void qu_special_parse(struct qu_context *ctx,
    const char *name, qu_ast_node *node);

#endif  // QUIRE_H_GEN_SPECIAL

