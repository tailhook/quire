#include <vars.h>
#ifndef _H_YPARSER
#define _H_YPARSER

#include <obstack.h>
#include <sys/queue.h>
#include <setjmp.h>

struct qu_parse_context_s;

#include "maputil.h"


#define MAX_FLOW_STACK 128
#define MAX_NODE_STACK 256


typedef struct qu_token_s {
    CIRCLEQ_ENTRY(qu_token_s) lst;
    int kind;

    char *filename;
    int indent;  // indentation, -1 if middle of the line or flow context
    int expect_indent;
    int start_line;
    int start_char;
    int end_line;
    int end_char;
    unsigned char *data;  // real pointer to data
    int bytepos;  // byte offset from start of file
    int bytelen;  // length in bytes

    struct qu_node_s *owner_node;
} qu_token;

typedef struct qu_node_s {
    int kind;
    struct parse_context_s *ctx;
    qu_token *anchor;
    qu_token *tag;
    qu_token *start_token;
    qu_token *end_token;

    char *content;  // for scalar nodes or aliases or mappings with "="
    int content_len;

    union {
        struct qu_node_s *alias_target;
        qu_map_index map_index;
        qu_seq_index seq_index;
    } val;

    void *userdata;
} qu_ast_node;

typedef struct parse_context_s {
    struct obstack pieces;
    jmp_buf errjmp;
    int has_jmp;
    char *filename;
    unsigned char *buf;
    int buflen;

    CIRCLEQ_HEAD(qu_token_list, qu_token_s) tokens;
    qu_ast_node *document;
    qu_variable_t *variables;

    int linestart; // boolean flag if we are still at indentation
    int curline;
    int curpos;
    int indent;
    unsigned char *ptr;
    int flow_num;
    char flow_stack[MAX_FLOW_STACK];

    int cur_mapping;
    int cur_sequence;

    qu_token *cur_token;
    qu_token *cur_anchor;
    qu_token *cur_tag;

    int error_kind;
    char *error_text;
    qu_token *error_token;

} qu_parse_context;


// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
int qu_file_parse(qu_parse_context *ctx, char *filename);
void qu_context_free(qu_parse_context *ctx);


#endif // _H_YPARSER
