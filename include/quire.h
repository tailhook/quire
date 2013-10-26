#ifndef QUIRE_MAIN_INCLUDED
#define QUIRE_MAIN_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <setjmp.h>

// Constants from emitter.h
#define QU_EMIT_UNKNOWN     0
#define QU_EMIT_MAP_START   1
#define QU_EMIT_MAP_END     2
#define QU_EMIT_MAP_KEY     3
#define QU_EMIT_MAP_VALUE   4
#define QU_EMIT_SEQ_START   5
#define QU_EMIT_SEQ_END     6
#define QU_EMIT_SEQ_ITEM    7

#define QU_WS_SPACE         0
#define QU_WS_INDENT        1
#define QU_WS_ENDLINE       2

#define QU_STYLE_AUTO       0
#define QU_STYLE_PLAIN      1
#define QU_STYLE_DOUBLE     2
#define QU_STYLE_SINGLE     3
#define QU_STYLE_LITERAL    4
#define QU_STYLE_FOLDED     5

#define QU_COMMENT_NORMAL    0
#define QU_COMMENT_REWRAP    1
#define QU_COMMENT_NOINDENT  2
#define QU_COMMENT_SAME_LINE 3
#define QU_COMMENT_NICE      (-1)

#define QU_FLAGS_VARS       1

#define QU_CLI_RUN          0
#define QU_CLI_CHECK_CONFIG 1
#define QU_CLI_PRINT_CONFIG 2
#define QU_CLI_PRINT_HELP   3

#define QU_PRINT_EXAMPLE  1
#define QU_PRINT_COMMENTS 2
#define QU_PRINT_FULL     4

/*  A value random enough to not to conflict with any errno value  */
#define QU_YAML_ERROR       97156

#define QU_ERR_UNKNOWN 0
#define QU_ERR_SYNTAX  1
#define QU_ERR_SYSTEM  2
#define QU_ERR_ERRNO   3
#define QU_ERR_VALUE   4
#define QU_ERR_CMDLINE 5

typedef struct qu_config_head {
    char data[512];
} qu_config_head;

typedef struct qu_emit_context {
    char data[512];
} qu_emit_context;

struct qu_short_option {
    char opt;
    int has_arg;
    int name;
};

struct qu_long_option {
    const char *opt;
    int has_arg;
    int name;
};

struct qu_config_context;

typedef struct qu_ast_node qu_ast_node;
typedef struct qu_seq_member qu_seq_member;
typedef struct qu_map_member qu_map_member;

// Methods from access.c
qu_ast_node *qu_config_root(struct qu_config_context *ctx);
qu_ast_node *qu_map_get(qu_ast_node *node, char *key);
char *qu_node_content(qu_ast_node *node);
const char *qu_node_tag(qu_ast_node *node);
qu_seq_member *qu_seq_iter(qu_ast_node *node);
qu_seq_member *qu_seq_next(qu_seq_member *iter);
qu_ast_node *qu_seq_node(qu_seq_member *iter);
qu_map_member *qu_map_iter(qu_ast_node *node);
qu_map_member *qu_map_next(qu_map_member *iter);
qu_ast_node *qu_map_key(qu_map_member *iter);
qu_ast_node *qu_map_value(qu_map_member *iter);

void *qu_config_init(void *cfg, int size);
void qu_config_free(void *cfg);
void *qu_config_alloc(struct qu_config_context *cfg, int size);

// Methods from cfg/api.c
struct qu_config_context *qu_config_parser(struct qu_config_head *head,
    jmp_buf *jmp);
void qu_config_parser_free(struct qu_config_context *ctx);
qu_ast_node *qu_config_parse_yaml(struct qu_config_context *ctx,
                                  const char *filename);

// Methods from cfg/optparser.c
void qu_optparser_init(struct qu_config_context *ctx,
    int argc, char **argv,
    const struct qu_short_option *shopt,
    const struct qu_long_option *lopt);
int qu_optparser_next(struct qu_config_context *ctx,
    int *opt, const char **arg);
void qu_optparser_error(struct qu_config_context *ctx,
    const char *error);

// Methods from eval.c
void qu_node_to_bool(struct qu_config_context *ctx, qu_ast_node *node,
    int *result);
void qu_node_to_int(struct qu_config_context *ctx, qu_ast_node *node,
    long *result);
void qu_node_to_float(struct qu_config_context *ctx, qu_ast_node *node,
    double *result);
void qu_node_to_str(struct qu_config_context *ctx, qu_ast_node *node,
    const char **result, int *rlen);

// Methods from cfg/vars.h
int qu_set_string(struct qu_config_context *ctx, const char *name, const char *data);
int qu_set_integer(struct qu_config_context *ctx, const char *name, long value);

// Methods from error.c
void qu_print_error(struct qu_config_context *, FILE *err);
void qu_report_error(struct qu_config_context *, qu_ast_node *node,
    const char *text);
void qu_cli_error(struct qu_config_context *, const char *opt,
    const char *text);

// Methods from emitter.c
int qu_emit_init(qu_emit_context *, FILE *stream);
int qu_emit_done(qu_emit_context *);

int qu_emit_scalar(qu_emit_context *, const char *tag, const char *anchor,
    int style, const char *data, int len);
int qu_emit_printf(qu_emit_context *, const char *tag, const char *anchor,
    int style, const char *format, ...);
int qu_emit_comment(qu_emit_context *, int flags, const char *data, int len);
int qu_emit_commentf(qu_emit_context *, int flags, const char *format, ...);
int qu_emit_whitespace(qu_emit_context *, int kind, int count);
int qu_emit_opcode(qu_emit_context *, const char *tag, const char *anchor,
    int code);
int qu_emit_alias(qu_emit_context *, const char *name);


#endif // QUIRE_MAIN_INCLUDED
