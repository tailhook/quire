#ifndef QUIRE_H_GEN_UTIL_FWDECL
#define QUIRE_H_GEN_UTIL_FWDECL

struct qu_context;

struct qu_fwdecl_index {
    struct qu_fwdecl_node *root;
};

int qu_fwdecl_add(struct qu_context *ctx, const char *name);
void qu_fwdecl_print_all(struct qu_context *ctx);

#endif  // QUIRE_H_GEN_UTIL_FWDECL
