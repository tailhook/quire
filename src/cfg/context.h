#ifndef QUIRE_H_CFG_CONTEXT
#define QUIRE_H_CFG_CONTEXT

#include "../yaml/parser.h"
#include "vars.h"

typedef struct qu_config_context {
    qu_parse_context parser;
    struct qu_vars_index variables;
} qu_config_context;

void qu_config_context_init(struct qu_config_context *ctx, jmp_buf *jmp);
void qu_config_context_free(struct qu_config_context *ctx);

#endif // QUIRE_H_CFG_CONTEXT
