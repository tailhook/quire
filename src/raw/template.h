#ifndef QUIRE_H_RAW_TEMPLATE
#define QUIRE_H_RAW_TEMPLATE

#include "common.h"
#include "../yaml/node.h"

struct qu_parser;

qu_ast_node *qu_raw_template(struct qu_parser *ctx, qu_ast_node *node);

#endif  /* QUIRE_H_RAW_TEMPLATE */
