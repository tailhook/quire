#ifndef QUIRE_H_CFG_CONTEXT
#define QUIRE_H_CFG_CONTEXT

#include "../yaml/parser.h"
#include "vars.h"
#include "optparser.h"
#include "../error.h"

struct qu_config_head;

typedef struct qu_config_context {
    struct qu_errbuf errbuf;
    struct qu_parser parser;
    struct qu_vars_index variables;
    struct qu_optparser_struct optparser;
    struct qu_errbuf *err;
    struct obstack *alloc;
} qu_config_context;

void qu_config_context_init(struct qu_config_context *ctx,
    struct qu_config_head *target, jmp_buf *jmp);
void qu_config_context_free(struct qu_config_context *ctx);

#endif // QUIRE_H_CFG_CONTEXT
