#ifndef _H_CODES
#define _H_CODES

// Value types
#define QU_TYP_UNKNOWN  0
#define QU_TYP_INT      1
#define QU_TYP_FLOAT    2
#define QU_TYP_FILE     3
#define QU_TYP_DIR      4
#define QU_TYP_STRING   5
#define QU_TYP_BOOL     6
#define QU_TYP_ARRAY    7
#define QU_TYP_MAP      8
#define QU_TYP_CUSTOM   9

// Map merge flags
#define QU_MFLAG_MAPMERGE 1
#define QU_MFLAG_SEQMERGE 2
#define QU_MFLAG_RESOLVEALIAS 4
// Include flags
#define QU_IFLAG_FROMFILE 1
#define QU_IFLAG_INCLUDE  2
#define QU_IFLAG_GLOBSEQ  3
#define QU_IFLAG_GLOBMAP  4

#define QU_YAML_ERROR       97156

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
    YAML_CONTENT_ERROR,
    YAML_SYSTEM_ERROR,
    YAML_UNSUPPORTED_ERROR
} qu_error_t;

typedef enum qu_member_enum {
    QU_MEMBER_NONE,
    QU_MEMBER_STRUCT,
    QU_MEMBER_SCALAR,
    QU_MEMBER_ARRAY,
    QU_MEMBER_MAP,
    QU_MEMBER_CUSTOM,
} qu_member_t;

#endif // _H_CODES
