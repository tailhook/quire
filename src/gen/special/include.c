#include "include.h"
#include "../context.h"
#include "../util/print.h"
#include "../../yaml/access.h"

struct qu_include {
    TAILQ_ENTRY(qu_include) lst;
    const char *name;
};

void qu_special_include(struct qu_context *ctx, qu_ast_node *fnnode) {
    struct qu_include *self;
    self = obstack_alloc(&ctx->parser.pieces, sizeof(struct qu_include));
    self->name = qu_node_content(fnnode);
    TAILQ_INSERT_TAIL(&ctx->includes.list, self, lst);
}

void qu_include_init(struct qu_context *ctx) {
    TAILQ_INIT(&ctx->includes.list);
}

void qu_include_print(struct qu_context *ctx) {
    struct qu_include *item;
    TAILQ_FOREACH(item, &ctx->includes.list, lst) {
        qu_code_print(ctx,
            "#include ${fn:q}\n"
            , "fn", item->name
            , NULL);
    }
}
