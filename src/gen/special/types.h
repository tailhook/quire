#ifndef QUIRE_H_GEN_SPECIAL_TYPES
#define QUIRE_H_GEN_SPECIAL_TYPES

#include "../../yaml/node.h"

struct qu_context;

struct qu_class_index {
    struct qu_class *root;
};

void qu_special_types(struct qu_context *ctx, qu_ast_node *typesnode);
void *qu_class_get_data(struct qu_class *cls);
void qu_class_set_data(struct qu_class *cls, void *data);
void qu_classes_init(struct qu_context *ctx);
struct qu_class *qu_class_get(struct qu_context *ctx, const char *name);

#endif  // QUIRE_H_GEN_SPECIAL_TYPES
