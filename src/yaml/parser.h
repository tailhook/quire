#ifndef QUIRE_H_YAML_PARSER
#define QUIRE_H_YAML_PARSER

#include <obstack.h>
#include <sys/queue.h>
#include <setjmp.h>

struct qu_parse_context_s;

#include "node.h"
#include "map.h"
#include "anchors.h"

#define MAX_FLOW_STACK 128
#define MAX_NODE_STACK 256



typedef struct qu_parse_context_s {
    struct obstack pieces;
    jmp_buf *errjmp;
    char *filename;
    unsigned char *buf;
    int buflen;

    CIRCLEQ_HEAD(qu_token_list, qu_token_s) tokens;
    qu_ast_node *document;
    struct qu_anchor_index anchor_index;

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
    const char *error_text;
    qu_token *error_token;

} qu_parse_context;

qu_ast_node *qu_file_newparse(qu_parse_context *ctx, const char *filename);

void qu_file_parse(qu_parse_context *ctx, const char *filename);
void qu_parser_init(qu_parse_context *ctx);
void qu_parser_free(qu_parse_context *ctx);


#endif // QUIRE_H_YAML_PARSER
