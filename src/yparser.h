#ifndef _H_YPARSER
#define _H_YPARSER

#include <obstack.h>
#include <sys/queue.h>


#define MAX_FLOW_STACK 128
#define MAX_NODE_STACK 256

typedef enum token_kind {
    // Keep in sync with strings in yparser.c
    TOKEN_ERROR,
    TOKEN_DOC_START,
    TOKEN_DOC_END,
    TOKEN_INDENT,
    TOKEN_WHITESPACE,
    TOKEN_PLAINSTRING,
    TOKEN_SINGLESTRING,
    TOKEN_DOUBLESTRING,
    TOKEN_LITERAL,
    TOKEN_FOLDED,
    TOKEN_COMMENT,
    TOKEN_TAG,
    TOKEN_ALIAS,
    TOKEN_ANCHOR,
    TOKEN_SEQUENCE_ENTRY,  // '-'
    TOKEN_MAPPING_KEY,  // '?'
    TOKEN_MAPPING_VALUE,  // ':'
    TOKEN_FLOW_SEQ_START,  // '['
    TOKEN_FLOW_SEQ_END,  // ']'
    TOKEN_FLOW_MAP_START,  // '{'
    TOKEN_FLOW_MAP_END,  // '}'
    TOKEN_FLOW_ENTRY,  // ','
    TOKEN_DIRECTIVE,  // '%...'
    TOKEN_RESERVED,  // '@' or '`'
} token_kind;

typedef enum node_kind {
    NODE_UNKNOWN,
    NODE_ALIAS,
    NODE_EMPTY,
    NODE_SCALAR,
    NODE_SEQUENCE,
    NODE_MAPPING,
} node_kind;

typedef struct token_s {
    CIRCLEQ_ENTRY(token_s) lst;
    int kind;

    char *filename;
    int indent;  // indentation, -1 if middle of the line or flow context
    int start_line;
    int start_char;
    int end_line;
    int end_char;
    unsigned char *data;  // real pointer to data
    int bytepos;  // byte offset from start of file
    int bytelen;  // length in bytes

    struct node_s *owner_node;
} yaml_token;

typedef struct node_s{
    CIRCLEQ_ENTRY(node_s) lst;

    int kind;
    yaml_token *anchor;
    yaml_token *tag;
    yaml_token *start_token;
    yaml_token *end_token;

    char *content;  // for scalar nodes or aliases
    int content_len;

    CIRCLEQ_HEAD(ast_children, node_s) children; // for container nodes
} yaml_ast_node;

typedef struct parse_context_s {
    struct obstack pieces;
    char *filename;
    unsigned char *buf;
    int buflen;

    CIRCLEQ_HEAD(token_list, token_s) tokens;
    yaml_ast_node *document;

    int linestart; // boolean flag if we are still at indentation
    int curline;
    int curpos;
    int indent;
    unsigned char *ptr;
    int flow_num;
    char flow_stack[MAX_FLOW_STACK];

    yaml_token *cur_token;

    int error_kind;
    char *error_text;
    yaml_token *error_token;

} yaml_parse_context;


void yaml_init();
int yaml_context_init(yaml_parse_context *ctx);
int yaml_load_file(yaml_parse_context *ctx, char *filename);
int yaml_tokenize(yaml_parse_context *ctx);
int yaml_parse(yaml_parse_context *ctx);
int yaml_context_free(yaml_parse_context *ctx);


#endif // _H_YPARSER
