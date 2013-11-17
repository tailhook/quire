#ifndef QUIRE_H_RAW_MERGE
#define QUIRE_H_RAW_MERGE

#include "../yaml/parser.h"

void qu_raw_maps_visitor(struct qu_parser *ctx, qu_ast_node *node,
    unsigned flags);

#endif // QUIRE_H_RAW_MERGE
