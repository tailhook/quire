#ifndef QUIRE_H_GEN_CLASSES
#define QUIRE_H_GEN_CLASSES

#include "../special/types.h"

struct qu_context;
struct qu_option;

struct qu_class {
    const char *name;
    qu_token *start_token;  /*  This is to store file/line info  */
    int line;
    struct qu_class_vptr *vp;
    void *classdata;
    struct qu_class *left;
    struct qu_class *right;
    int has_default;
};

struct qu_class_vptr {
    void (*init)(struct qu_context *, struct qu_class *, qu_ast_node *node);
    void (*var_decl)(struct qu_context *, struct qu_class *,
        struct qu_option *opt, const char *varname);
    void (*func_decl)(struct qu_context *, struct qu_class *);
    void (*func_body)(struct qu_context *, struct qu_class *);
};

struct qu_class_vptr *qu_class_get_vptr(const char *name);
void qu_classes_print_functions(struct qu_context *ctx);

#endif  /* QUIRE_H_GEN_CLASSES */
