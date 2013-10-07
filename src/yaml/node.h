#ifndef QUIRE_H_YAML_NODE
#define QUIRE_H_YAML_NODE

#include "map.h"
#include "seq.h"

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
    struct qu_parse_context_s *ctx;
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

#endif // QUIRE_H_YAML_NODE
