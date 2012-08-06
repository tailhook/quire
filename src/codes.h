#ifndef _H_CODES
#define _H_CODES

typedef enum qu_token_kind_enum {
    // Keep in sync with strings in yparser.c
    QU_TOK_ERROR,
    QU_TOK_DOC_START,
    QU_TOK_DOC_END,
    QU_TOK_INDENT,
    QU_TOK_WHITESPACE,
    QU_TOK_PLAINSTRING,
    QU_TOK_SINGLESTRING,
    QU_TOK_DOUBLESTRING,
    QU_TOK_LITERAL,
    QU_TOK_FOLDED,
    QU_TOK_COMMENT,
    QU_TOK_TAG,
    QU_TOK_ALIAS,
    QU_TOK_ANCHOR,
    QU_TOK_SEQUENCE_ENTRY,  // '-'
    QU_TOK_MAPPING_KEY,  // '?'
    QU_TOK_MAPPING_VALUE,  // ':'
    QU_TOK_FLOW_SEQ_START,  // '['
    QU_TOK_FLOW_SEQ_END,  // ']'
    QU_TOK_FLOW_MAP_START,  // '{'
    QU_TOK_FLOW_MAP_END,  // '}'
    QU_TOK_FLOW_ENTRY,  // ','
    QU_TOK_DIRECTIVE,  // '%...'
    QU_TOK_RESERVED,  // '@' or '`'
} yu_token_kind_t;

typedef enum qu_node_kind_enum {
    QU_NODE_UNKNOWN,
    QU_NODE_ALIAS,
    QU_NODE_EMPTY,
    QU_NODE_SCALAR,
    QU_NODE_SEQUENCE,
    QU_NODE_MAPPING,
} qu_node_kind_t;

typedef enum qu_error_enum {
    YAML_NO_ERROR,
    YAML_SCANNER_ERROR,
    YAML_PARSER_ERROR,
    YAML_CONTENT_ERROR
} qu_error_t;

#endif // _H_CODES
