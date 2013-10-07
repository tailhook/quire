#ifndef QUIRE_H_CFG_CONTEXT
#define QUIRE_H_CFG_CONTEXT

#include "../yaml/parser.h"
#include "vars.h"

typedef struct qu_config_context {
    qu_parse_context parser __attribute__((aligned(4096)));
    struct qu_vars_index variables;
} qu_config_context;

#endif // QUIRE_H_CFG_CONTEXT
