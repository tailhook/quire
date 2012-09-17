#ifndef QUIRE_MAIN_INCLUDED
#define QUIRE_MAIN_INCLUDED

// Constants from emitter.h
#define QU_EMIT_UNKNOWN     0
#define QU_EMIT_MAP_START   1
#define QU_EMIT_MAP_END     2
#define QU_EMIT_MAP_KEY     3
#define QU_EMIT_MAP_VALUE   4
#define QU_EMIT_SEQ_START   5
#define QU_EMIT_SEQ_END     6

#define QU_WS_SPACE         0
#define QU_WS_INDENT        1
#define QU_WS_ENDLINE       2

#define QU_STYLE_AUTO       0
#define QU_STYLE_PLAIN      1
#define QU_STYLE_DOUBLE     2
#define QU_STYLE_SINGLE     3
#define QU_STYLE_LITERAL    4
#define QU_STYLE_FOLDED     5

#define QU_COMMENT_NORMAL   0
#define QU_COMMENT_REWRAP   1
#define QU_COMMENT_INDENT   2
#define QU_COMMENT_NICE     (-1)

typedef struct qu_config_head {
    char data[128];
} qu_config_head;

typedef struct qu_parse_context {
    char data[4096];
} qu_parse_context;

typedef struct qu_emit_context {
    char data[512];
} qu_emit_context;

typedef struct qu_ast_node qu_ast_node;

// Methods from access.c
qu_ast_node *qu_get_root(qu_parse_context *ctx);
qu_ast_node *qu_map_get(qu_ast_node *node, char *key);
int qu_get_boolean(qu_ast_node *node, int *value);
char *qu_node_content(qu_ast_node *node);

// Methods from yparser.c
int qu_file_parse(qu_parse_context *ctx, char *filename)
    __attribute__((warn_unused_result));
void qu_context_free(qu_parse_context *ctx);

// Methods from error.c
int qu_has_error(qu_parse_context *);
int qu_print_error(qu_parse_context *, FILE *err);

// Methods from emitter.c
int qu_emit_init(qu_emit_context *, FILE *stream);
int qu_emit_done(qu_emit_context *);

int qu_emit_scalar(qu_emit_context *, char *tag, char *anchor,
    int style, char *data, int len);
int qu_emit_printf(qu_emit_context *, char *tag, char *anchor,
    int style, char *format, ...);
int qu_emit_comment(qu_emit_context *, int flags, char *data, int len);
int qu_emit_commentf(qu_emit_context *, int flags, char *format, ...);
int qu_emit_whitespace(qu_emit_context *, int kind, int count);
int qu_emit_opcode(qu_emit_context *, char *tag, char *anchor,
    int code);
int qu_emit_alias(qu_emit_context *, char *name);


#endif // QUIRE_MAIN_INCLUDED
