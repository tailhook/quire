#ifndef QUIRE_MAIN_INCLUDED
#define QUIRE_MAIN_INCLUDED

typedef struct qu_config_head {
    char data[128];
} qu_config_head;

typedef struct qu_parse_context {
    char data[4096];
} qu_parse_context;

typedef struct qu_ast_node qu_ast_node;

// Methods from access.c
qu_ast_node *qu_get_root(qu_parse_context *ctx);
qu_ast_node *qu_map_get(qu_ast_node *node, char *key);
int qu_get_boolean(qu_ast_node *node, int *value);
char *qu_node_content(qu_ast_node *node);

// Methods from yparser.c
void qu_init();
int qu_context_init(qu_parse_context *ctx);
int qu_load_file(qu_parse_context *ctx, char *filename);
int qu_tokenize(qu_parse_context *ctx);
int qu_parse(qu_parse_context *ctx);
int qu_context_free(qu_parse_context *ctx);

// Methods from error.c
int qu_has_error(qu_parse_context *);
int qu_print_error(qu_parse_context *, FILE *err);


#endif // QUIRE_MAIN_INCLUDED
