#ifndef QUIRE_H_YAML_ANCHORS
#define QUIRE_H_YAML_ANCHORS

#include "node.h"

struct qu_parser;

struct qu_anchor_index {
    struct qu_anchor_node *node;
};

void qu_insert_anchor(struct qu_parser *ctx, qu_ast_node *node);
qu_ast_node *qu_find_anchor(struct qu_parser *ctx,
    const char *name, int name_len);

#endif // QUIRE_H_YAML_ANCHORS
