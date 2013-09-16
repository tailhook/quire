#ifndef QUIRE_MAIN_INCLUDED
#define QUIRE_MAIN_INCLUDED

#include <stdint.h>
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

#define QU_COMMENT_NORMAL   0
#define QU_COMMENT_REWRAP   1
#define QU_COMMENT_INDENT   2
#define QU_COMMENT_NICE     (-1)

#define QU_FLAGS_VARS       1

#define QU_MODE_NORMAL      1

typedef struct qu_config_head {
	jmp_buf safejump;
    char data[512 - sizeof(jmp_buf)];
} qu_config_head;

typedef struct qu_parse_context {
    char data[4096];
} qu_parse_context;

typedef struct qu_emit_context {
    char data[512];
} qu_emit_context;

typedef struct qu_array_head {
	void *_data[2];
} qu_array_head;

typedef struct qu_mapping_head {
	void *_data[2];
} qu_mapping_head;

typedef struct qu_ast_node qu_ast_node;
typedef struct qu_seq_member qu_seq_member;
typedef struct qu_map_member qu_map_member;

// Methods from access.c
qu_ast_node *qu_get_root(qu_parse_context *ctx);
qu_ast_node *qu_map_get(qu_ast_node *node, char *key);
int qu_get_boolean(qu_ast_node *node, int *value);
char *qu_node_content(qu_ast_node *node);
qu_seq_member *qu_seq_iter(qu_ast_node *node);
qu_seq_member *qu_seq_next(qu_seq_member *iter);
qu_ast_node *qu_seq_node(qu_seq_member *iter);
qu_map_member *qu_map_iter(qu_ast_node *node);
qu_map_member *qu_map_next(qu_map_member *iter);
qu_ast_node *qu_map_key(qu_map_member *iter);
qu_ast_node *qu_map_value(qu_map_member *iter);
void qu_get_tag(qu_ast_node *node, char **data, int *len);

void *qu_config_init(void *cfg, int size);
void qu_config_free(void *cfg);
void *qu_config_alloc(void *cfg, int size);
void qu_config_array_insert(void **head, void **tail,
							int *list_size,
						    qu_array_head *member);
void *qu_config_array_next(void *elem);
void qu_config_mapping_insert(void **head, void **tail,
							int *list_size,
						    qu_mapping_head *member);
void *qu_config_mapping_next(void *elem);

// Methods from yparser.c
int qu_file_parse(qu_parse_context *ctx, char *filename)
    __attribute__((warn_unused_result));
void qu_parser_init(qu_parse_context *ctx);
void qu_parser_free(qu_parse_context *ctx);

// Methods from eval.c
void qu_node_to_int(qu_parse_context *ctx, qu_ast_node *node, uint64_t flags,
    long *result);
void qu_node_to_float(qu_parse_context *ctx, qu_ast_node *node, uint64_t flags,
    double *result);
void qu_node_to_str(qu_parse_context *ctx, qu_ast_node *node, uint64_t flags,
    char **result, size_t *rlen);

// Methods from vars.h
int qu_set_string(qu_parse_context *ctx, char*name, char *data);
int qu_set_integer(qu_parse_context *ctx, char *name, long value);

// Methods from error.c
int qu_has_error(qu_parse_context *);
int qu_print_error(qu_parse_context *, FILE *err);
void qu_report_error(qu_parse_context *, qu_ast_node *node, char *text);

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

// Methods & constants from maputil.c
#define QU_MFLAG_MAPMERGE 1
#define QU_MFLAG_SEQMERGE 2
#define QU_MFLAG_RESOLVEALIAS 4
int qu_merge_maps(qu_parse_context *ctx, int flags);
#define QU_IFLAG_FROMFILE 1
#define QU_IFLAG_INCLUDE  2
#define QU_IFLAG_GLOBSEQ  3
#define QU_IFLAG_GLOBMAP  4
int qu_process_includes(qu_parse_context *ctx, int flags);


#endif // QUIRE_MAIN_INCLUDED
