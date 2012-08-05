#ifndef _H_METADATA
#define _H_METADATA

#include "yparser.h"

typedef struct coyaml_metadata_s {
    char *program_name;
    char *default_config;
    char *description;
    int has_arguments;
    int mixed_arguments;
} coyaml_metadata_t;

int parse_metadata(yaml_ast_node *root, coyaml_metadata_t *meta);

#endif // _H_METADATA
