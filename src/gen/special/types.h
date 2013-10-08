#ifndef QUIRE_H_GEN_SPECIAL_TYPES
#define QUIRE_H_GEN_SPECIAL_TYPES

#include "../../yaml/node.h"

struct qu_context;

struct qu_class_index {
    struct qu_class *root;
};

void qu_special_types(struct qu_context *ctx, qu_ast_node *typesnode);

#endif  // QUIRE_H_GEN_SPECIAL_TYPES
