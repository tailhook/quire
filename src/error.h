#ifndef QUIRE_H_ERROR
#define QUIRE_H_ERROR

#include <stdio.h>

#include "yaml/parser.h"

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
void qu_print_error(qu_parse_context *, FILE *err);
void qu_report_error(qu_parse_context *, qu_ast_node *, const char *text);
void qu_cmdline_error(qu_parse_context *ctx, const char *opt, const char *text);

#endif  /* QUIRE_H_ERROR */
