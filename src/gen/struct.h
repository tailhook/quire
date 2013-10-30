#ifndef QUIRE_H_GEN_STRUCT
#define QUIRE_H_GEN_STRUCT

#include <sys/queue.h>

struct qu_context;

struct qu_struct_member {
    const char *name;
    int is_struct;
    TAILQ_ENTRY(qu_struct_member) lst;
    union {
        struct qu_option *opt;
        struct qu_config_struct *str;
    } p;
};

struct qu_config_struct {
    TAILQ_HEAD(qu_config_s_list, qu_struct_member) children;
    struct qu_config_struct *parent;
    const char *path;
};

struct qu_config_struct *qu_struct_new_root(struct qu_context *ctx);
struct qu_config_struct *qu_struct_substruct(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name);
void qu_struct_add_option(struct qu_context *ctx,
    struct qu_config_struct *parent, const char *name,
    struct qu_option *option);
void qu_struct_parser(struct qu_context *ctx, struct qu_config_struct *str,
    const char *prefix, int level);
void qu_struct_printer(struct qu_context *ctx, struct qu_config_struct *str,
    const char *prefix, const char *tag);
void qu_struct_definition(struct qu_context *ctx, struct qu_config_struct *str);
void qu_struct_default_setter(struct qu_context *ctx,
    struct qu_config_struct *str, const char *prefix);


#endif  // QUIRE_H_GEN_STRUCT
