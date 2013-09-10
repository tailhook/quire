#ifndef _H_VARS
#define _H_VARS

typedef enum qu_vartype_enum {
    QU_VAR_ANCHOR,
    QU_VAR_STRING,
    QU_VAR_INTEGER,
} qu_vartype_t;

typedef struct qu_variable_s {
    struct qu_variable_s *left;
    struct qu_variable_s *right;
    char *name;
    int name_len;
    qu_vartype_t type;
    union qu_variable_data {
        struct {
            struct qu_node_s *node;
        } anchor;
        struct {
            char *value;
            int value_len;
        } string;
        struct {
            long value;
        } integer;
    } data;
} qu_variable_t;

#include "yparser.h"

int qu_get_string(qu_parse_context *ctx, char*name, char **data, int *dlen);
int qu_print_variables(qu_parse_context *ctx);
void _qu_insert_anchor(qu_parse_context *ctx,
    unsigned char *name, int namelen, qu_ast_node *node);
qu_ast_node *_qu_find_anchor(qu_parse_context *ctx,
    unsigned char *name, int namelen);


// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
int qu_set_string(qu_parse_context *ctx, char*name, char *data);
int qu_set_integer(qu_parse_context *ctx, char*name, long value);

#endif //_H_VARS
