#ifndef QUIRE_H_GEN_SPECIAL_INCLUDE
#define QUIRE_H_GEN_SPECIAL_INCLUDE

#include <sys/queue.h>
#include "../../yaml/node.h"

struct qu_context;

struct qu_includes {
    TAILQ_HEAD(qu_include_lst, qu_include) list;
};

void qu_special_include(struct qu_context *ctx, qu_ast_node *fnnode);
void qu_include_init(struct qu_context *ctx);
void qu_include_print(struct qu_context *ctx);

#endif /*  QUIRE_H_GEN_SPECIAL_INCLUDE */
