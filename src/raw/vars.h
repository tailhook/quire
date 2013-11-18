#ifndef QUIRE_H_RAW_VARS
#define QUIRE_H_RAW_VARS

#include "../yaml/node.h"

struct obstack;
struct qu_parser;
struct qu_var_frame;
struct qu_variable;

void qu_var_set_string(struct obstack *buf, struct qu_var_frame *frame,
    const char *name, int nlen, const char *data, int dlen);
int qu_var_get_string(struct qu_var_frame *frame,
    const char *name, int name_len, const char **data, int *dlen);
struct qu_var_frame *qu_vars_frame(struct obstack *buf,
    struct qu_var_frame *parent);
struct qu_var_frame *qu_anchors_frame(struct obstack *buf,
    struct qu_parser *ctx, struct qu_var_frame *parent);
struct qu_var_frame *qu_node_frame(struct obstack *buf,
    qu_ast_node *node, struct qu_var_frame *parent);

#endif  /* QUIRE_H_RAW_VARS */
