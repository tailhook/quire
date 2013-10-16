#ifndef QUIRE_H_GEN_TYPES
#define QUIRE_H_GEN_TYPES

#include "../../yaml/node.h"

struct qu_context;
struct qu_option;

struct qu_option_vptr {
    /*  Parse yaml node, NULL for bare command-line options  */
    void (*parse)(struct qu_context *ctx,
        struct qu_option *opt, qu_ast_node *node);

    struct qu_cli_action *(*cli_action)(struct qu_option *opt,
        const char *action);
    void (*cli_parser)(struct qu_context *ctx,
        struct qu_option *opt, const char *action, const char *paramname);
    void (*parser)(struct qu_context *ctx,
        struct qu_option *opt, const char *expression, int level);
    void (*definition)(struct qu_context *ctx,
        struct qu_option *opt, const char *varname);
    void (*printer)(struct qu_context *ctx,
        struct qu_option *opt, const char *expression);
    void (*default_setter)(struct qu_context *ctx,
        struct qu_option *opt, const char *expression);
};

struct qu_option {
    struct qu_option_vptr *vp;
    const char *typname;
    const char *description;
    const char *path;
    qu_ast_node *example;
    int has_default;
    void *typedata;
};

struct qu_option *qu_option_new(struct qu_context *ctx,
    struct qu_option_vptr *vp);
struct qu_option *qu_option_resolve(struct qu_context *ctx,
    const char *tag, int taglen);


#endif  // QUIRE_H_GEN_TYPES
