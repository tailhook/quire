#ifndef _H_VARS
#define _H_VARS

struct qu_config_context;

struct qu_variable;

struct qu_vars_index {
    struct qu_variable *root;
};

int qu_string_var(struct qu_config_context *ctx,
    const char *name, int name_len, const char **data, int *dlen);
int qu_anchor_var(struct qu_config_context *ctx,
    const char *name, int name_len, const char **data, int *dlen);
void qu_vars_init(struct qu_config_context *ctx);

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
int qu_set_string(struct qu_config_context *ctx, const char *name, const char *data);

#endif //_H_VARS
