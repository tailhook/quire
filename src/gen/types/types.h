#ifndef QUIRE_H_GEN_TYPES
#define QUIRE_H_GEN_TYPES

#include "../../yaml/node.h"

struct qu_context;
struct qu_option;

struct qu_option_vptr {
    void (*parse)(struct qu_context *ctx,
        struct qu_option *opt, qu_ast_node *node);
};

struct qu_option {
    struct qu_option_vptr *vp;
    void *typedata;
};

struct qu_option *qu_option_new(struct qu_context *ctx,
    struct qu_option_vptr *vp);
struct qu_option *qu_option_resolve(struct qu_context *ctx,
    const char *tag, int taglen);


#endif  // QUIRE_H_GEN_TYPES
