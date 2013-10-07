#ifndef QUIRE_H_GEN_METADATA
#define QUIRE_H_GEN_METADATA

typedef struct qu_metadata_s {
    char *program_name;
    char *default_config;
    char *description;
    int has_arguments;
    int mixed_arguments;
} qu_metadata_t;

struct qu_context_s;

void qu_parse_metadata(struct qu_context_s *ctx);

#endif // QUIRE_H_GEN_METADATA
