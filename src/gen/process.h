#ifndef QUIRE_H_GEN_PREPROC
#define QUIRE_H_GEN_PREPROC

#include "../yaml/node.h"

struct qu_config_struct;
struct qu_guard;

void qu_config_preprocess(struct qu_context *ctx);
struct qu_option *qu_parse_option(struct qu_context *ctx, qu_ast_node *node,
    const char *name, struct qu_config_struct *parent);
void qu_visit_struct_children(struct qu_context *ctx,
    qu_ast_node *node, struct qu_config_struct *str, struct qu_guard *guard);

#endif  // QUIRE_H_GEN_PREPROC

