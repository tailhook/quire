#ifndef QUIRE_H_GEN_UTIL_FWDECL
#define QUIRE_H_GEN_UTIL_FWDECL

#include <sys/queue.h>

struct qu_context;

typedef void (*qu_fwdecl_printer)(struct qu_context *ctx, void *);

struct qu_fwdecl_index {
    struct qu_fwdecl_node *root;
    TAILQ_HEAD(qu_fwdecl_node_list, qu_fwdecl_node) list;
};

void qu_fwdecl_init(struct qu_context *ctx);
int qu_fwdecl_add(struct qu_context *ctx, const char *name,
    qu_fwdecl_printer printer, void *data);
void qu_fwdecl_print_all(struct qu_context *ctx);

#endif  // QUIRE_H_GEN_UTIL_FWDECL
