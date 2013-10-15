#ifndef QUIRE_H_GEN_STRUCT
#define QUIRE_H_GEN_STRUCT

#include <sys/queue.h>

struct qu_context;

struct qu_struct_member {
    const char *name;
    int is_struct;
    TAILQ_ENTRY(qu_config_ss_list) lst;
    union {
        struct qu_option *opt;
        struct qu_config_struct *str;
    } p;
};

struct qu_config_struct {
    TAILQ_HEAD(qu_config_s_list, qu_config_struct) children;
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
    const char *prefix);
void qu_struct_definition(struct qu_context *ctx, struct qu_config_struct *str);


#endif  // QUIRE_H_GEN_STRUCT