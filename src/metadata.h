#ifndef _H_METADATA
#define _H_METADATA

#include "yparser.h"

typedef struct qu_metadata_s {
    char *program_name;
    char *default_config;
    char *description;
    int has_arguments;
    int mixed_arguments;
} qu_metadata_t;

int qu_parse_metadata(qu_ast_node *root, qu_metadata_t *meta);

#endif // _H_METADATA
