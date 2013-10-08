#ifndef QUIRE_H_GEN_METADATA
#define QUIRE_H_GEN_METADATA

typedef struct qu_metadata_s {
    const char *program_name;
    const char *default_config;
    const char *description;
    int has_arguments;
    int mixed_arguments;
} qu_metadata_t;

struct qu_context;

void qu_parse_metadata(struct qu_context *ctx);

#endif // QUIRE_H_GEN_METADATA
