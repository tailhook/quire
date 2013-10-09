#ifndef _H_VARS
#define _H_VARS

typedef struct qu_config_context qu_config_context;

typedef enum qu_vartype_enum {
    QU_VAR_ANCHOR,
    QU_VAR_STRING,
    QU_VAR_INTEGER,
} qu_vartype_t;

struct qu_variable;

struct qu_vars_index {
    struct qu_variable *root;
};

int qu_get_string(qu_config_context *ctx,
    const char *name, const char **data, int *dlen);
int qu_get_string_len(qu_config_context *ctx,
    const char *name, int name_len, const char **data, int *dlen);
int qu_print_variables(qu_config_context *ctx);
void qu_vars_init(struct qu_config_context *ctx);

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
int qu_set_string(qu_config_context *ctx, const char *name, const char *data);
int qu_set_integer(qu_config_context *ctx, const char *name, long value);

#endif //_H_VARS
