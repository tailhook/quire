#ifndef QUIRE_H_GEN_PREPROC
#define QUIRE_H_GEN_PREPROC

struct qu_config_struct;

void qu_config_preprocess(struct qu_context *ctx);
struct qu_option *qu_parse_option(struct qu_context *ctx, qu_ast_node *node,
    const char *name, struct qu_config_struct *parent);
void qu_visit_struct_children(struct qu_context *ctx,
    qu_ast_node *node, struct qu_config_struct *str);

#endif  // QUIRE_H_GEN_PREPROC

