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

struct qu_context_s;

void _qu_parse_metadata(struct qu_context_s *ctx);

#endif // _H_METADATA
