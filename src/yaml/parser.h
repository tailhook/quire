#ifndef QUIRE_H_YAML_PARSER
#define QUIRE_H_YAML_PARSER

#include <obstack.h>
#include <stdio.h>
#include <sys/queue.h>
#include <setjmp.h>

#include "node.h"
#include "map.h"
#include "anchors.h"

#define MAX_FLOW_STACK 128
#define MAX_NODE_STACK 256


struct qu_parser {
    struct qu_errbuf *err;
    struct obstack pieces;
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
    union {
        qu_token *token;
        const char *cli_option;
        int errnum;
    } err_ptr;

};

qu_ast_node *qu_file_newparse(struct qu_parser *ctx, const char *filename);

void qu_file_parse(struct qu_parser *ctx, const char *filename);
void qu_stream_parse(struct qu_parser *ctx, const char *filename, FILE *stream);
void qu_parser_init(struct qu_parser *ctx, struct qu_errbuf *err);
void qu_parser_free(struct qu_parser *ctx);


#endif // QUIRE_H_YAML_PARSER
