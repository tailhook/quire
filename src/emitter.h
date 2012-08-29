#ifndef _H_EMITTER
#define _H_EMITTER

#include <stdio.h>

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
#define QU_EMIT_UNKNOWN     0
#define QU_EMIT_MAP_START   1
#define QU_EMIT_MAP_END     2
#define QU_EMIT_SEQ_START   3
#define QU_EMIT_SEQ_END     4

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
// End of PUBLIC API


typedef struct qu_emit_context {
    // Settings
    int min_indent;
    int width;
    int canonical_tags;
    int always_quote;
    int always_flow;

    // State
    FILE *stream;
    int flow_level;
    int cur_indent;
    char ident_levels[255];
} qu_emit_context;

_Static_assert(sizeof(qu_emit_context) < 512,
    "Size of qu_emit_context is too large");


// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
int qu_emit_init(qu_emit_context *, FILE *stream);
int qu_emit_free(qu_emit_context *);

int qu_emit_scalar(qu_emit_context *, char *tag, char *anchor,
    int style, char *data, int len);
int qu_emit_comment(qu_emit_context *, int flags, char *data, int len);
int qu_emit_commentf(qu_emit_context *, int flags, char *format, ...);
int qu_emit_whitespace(qu_emit_context *, int kind, int count);
int qu_emit_opcode(qu_emit_context *, char *tag, char *anchor,
    int code);
int qu_emit_alias(qu_emit_context *, char *name);
// End of PUBLIC API


#endif //_H_EMITTER
