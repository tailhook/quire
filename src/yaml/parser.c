#include <obstack.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <setjmp.h>

#include "../error.h"
#include "parser.h"
#include "access.h"
#include "codes.h"
#include "anchors.h"


enum char_klass {
    CHAR_UNKNOWN,
    CHAR_INDICATOR,
    CHAR_WHITESPACE,
    CHAR_DIGIT,
};

enum char_flag {
    CHAR_PRINTABLE=1,
    CHAR_FLOW_INDICATOR=2,
    CHAR_URI=4,
    CHAR_TAG=8,
    CHAR_PLAIN=16,
    CHAR_PLAIN_FLOW=32,
};

typedef struct {
    char klass;
    char flags;
} charinfo;

#include "chars.c"

char *token_to_str[] = {
    // Keep in sync with enum in yparser.h
    "ERROR",
    "DOC_START",
    "DOC_END",
    "INDENT",
    "WHITESPACE",
    "PLAINSTRING",
    "SINGLESTRING",
    "DOUBLESTRING",
    "LITERAL",
    "FOLDED",
    "COMMENT",
    "TAG",
    "ALIAS",
    "ANCHOR",
    "SEQUENCE_ENTRY",  // '-'
    "MAPPING_KEY",  // '?'
    "MAPPING_VALUE",  // ':'
    "FLOW_SEQ_START",  // '['
    "FLOW_SEQ_END",  // ']'
    "FLOW_MAP_START",  // '{'
    "FLOW_MAP_END",  // '}'
    "FLOW_ENTRY",  // ','
    "DIRECTIVE",  // '%...'
    "RESERVED",  // '@' or '`'
};




static void safeprint(FILE *stream, unsigned char *data, int len) {
    unsigned char *c, *end;
    for(c = data, end = data + len; c < end; ++c) {
        if(chars[(int)*c].flags & CHAR_PRINTABLE) {
            if(*c == '\r') {
                fprintf(stream, "\\r");
            } else if(*c == '\n') {
                fprintf(stream, "\\n");
            } else {
                fputc(*c, stream);
            }
        } else {
            fprintf(stream, "\\x%02x", *c);
        }
    }
}

static void print_token(qu_token *tok, FILE *stream) {
    fprintf(stream, "%s:%d %s [%d]``", tok->filename, tok->start_line,
        token_to_str[tok->kind], tok->indent);
    safeprint(stream, tok->data, tok->bytelen);
    fprintf(stream, "''\n");
}


static void *parser_chunk_alloc(struct qu_parser *ctx, int size) {
    void *res = malloc(size);
    if(!res) {
        qu_err_system_fatal(ctx->err, ENOMEM, "Obstack error");
    }
    return res;
}

static void obstack_chunk_free(struct qu_parser *ctx, void *ptr) {
    (void) ctx;
    free(ptr);
}

static void _qu_context_reinit(struct qu_parser *ctx) {
    CIRCLEQ_INIT(&ctx->tokens);
    ctx->buf = NULL;
    ctx->error_kind = 0;
}

void qu_parser_init(struct qu_parser *ctx, struct qu_errbuf *err) {
    ctx->err = err;
    obstack_specify_allocation_with_arg(&ctx->pieces, 4096, 0,
        parser_chunk_alloc, obstack_chunk_free, ctx);
    ctx->anchor_index.node = NULL;
    _qu_context_reinit(ctx);
}

static void qu_load_stream(struct qu_parser *ctx, FILE *stream, const char *fn)
{
    int rc, eno;
    char chunk[4096];
    obstack_blank(&ctx->pieces, 0);
    while(1) {
        rc = fread(chunk, 1, sizeof(chunk), stream);
        if(rc <= 0) {
            if(feof(stream))
                break;
            if(ferror(stream)) {
                eno = errno;
                fclose(stream);
                qu_err_system_fatal(ctx->err, eno, "Can't read \"%s\"", fn);
            }
        }
        obstack_grow(&ctx->pieces, chunk, rc);
    }
    ctx->buflen = obstack_object_size(&ctx->pieces);
    obstack_1grow(&ctx->pieces, 0);
    ctx->buf = obstack_finish(&ctx->pieces);
}

void qu_parser_free(struct qu_parser *ctx) {
    obstack_free(&ctx->pieces, NULL);
}

static qu_token *init_token(struct qu_parser *ctx) {
    qu_token *ctok = obstack_alloc(&ctx->pieces, sizeof(qu_token));
    ctok->kind = QU_TOK_ERROR;
    CIRCLEQ_INSERT_TAIL(&ctx->tokens, ctok, lst);
    ctok->filename = ctx->filename;
    ctok->indent = ctx->indent;
    ctok->start_line = ctx->curline;
    ctok->start_char = ctx->curpos;
    ctok->end_line = ctx->curline;
    ctok->end_char = ctx->curpos;
    ctok->data = ctx->ptr;
    ctok->bytepos = ctx->ptr - ctx->buf;
    ctok->bytelen = 0;
    ctok->owner_node = NULL;
    return ctok;
}

void qu_tokenize(struct qu_parser *ctx) {

#define KLASS (chars[(int)CHAR].klass)
#define FLAGS (chars[(int)CHAR].flags)
#define NEXT_KLASS (chars[(int)NEXT_CHAR].klass)
#define NEXT_FLAGS (chars[(int)NEXT_CHAR].flags)
#define LOOP for(; ctx->ptr < end; ++ctx->ptr, ++ctx->curpos)
#define NEXT ((++ctx->curpos, ++ctx->ptr) < end)
#define FORCE_NEXT if((++ctx->curpos, ++ctx->ptr) >= end) { \
    SYNTAX_ERROR("Premature end of file"); \
    }
#define SYNTAX_ERROR(message) do {\
    ctok->kind = QU_TOK_ERROR; \
    ctok->end_line = ctx->curline; \
    ctok->end_char = ctx->curpos; \
    qu_err_file_fatal(ctx->err, ctok->filename, ctok->start_line, (message)); \
    } while(0);

#define CONSUME_WHILE(cond) LOOP { if(!(cond)) break; }
#define CONSUME_UNTIL(cond) LOOP { if(cond) break; }

#define CHAR (*ctx->ptr)
#define PREV_CHAR (*(ctx->ptr-1))  // use carefully!
#define NEXT_CHAR (*(ctx->ptr+1))  // safe, since we have \0 at the end

    unsigned char *end = ctx->buf + ctx->buflen;
    ctx->curline = 1;
    ctx->linestart = 1;
    ctx->curpos = 1;
    ctx->scalarindent = 0;
    ctx->indent = 0;
    ctx->ptr = ctx->buf;
    ctx->flow_num = 0;
    for(;ctx->ptr < end;) {
        qu_token *ctok = init_token(ctx);
        switch(KLASS) {
        case CHAR_INDICATOR:
            switch (CHAR) {
            // Indicators
            case '-': // list element, doc start, plainstring
                FORCE_NEXT;
                if(KLASS == CHAR_WHITESPACE) {
                    ctok->kind = QU_TOK_SEQUENCE_ENTRY;
                    if(!ctx->linestart) {
                        SYNTAX_ERROR("Sequence entry dash sign is only"
                                     " allowed at the start of line")
                    }
                    ctx->scalarindent = ctx->indent + 2;
                    ctok->indent = ctx->indent;
                    break;
                } else if(CHAR == '-' && NEXT && CHAR == '-') {
                    ctok->kind = QU_TOK_DOC_START;
                    (void)NEXT;
                    break;
                } else {
                    goto plainstring;
                }
            case '?': // key, plainstring
                FORCE_NEXT;
                if(KLASS == CHAR_WHITESPACE) {
                    ctx->linestart = 0;
                    ctx->scalarindent = ctx->indent + 2;
                    ctok->kind = QU_TOK_MAPPING_KEY;
                    break;
                } else {
                    goto plainstring;
                }
            case ':': // value, plainstring
                FORCE_NEXT;
                if(KLASS == CHAR_WHITESPACE) {
                    ctx->linestart = 0;
                    ctok->kind = QU_TOK_MAPPING_VALUE;
                    break;
                } else if(CIRCLEQ_PREV(ctok, lst) &&
                    CIRCLEQ_PREV(ctok, lst)->kind == QU_TOK_DOUBLESTRING) {
                    // JSON-like value {"abc":123}
                    ctok->kind = QU_TOK_MAPPING_VALUE;
                    break;
                } else {
                    goto plainstring;
                }
            case '%':
                ctok->kind = QU_TOK_DIRECTIVE;
                CONSUME_UNTIL(CHAR == '\n');
                break;
            case '@': case '`':
                ctok->end_char += 1;
                ctok->kind = QU_TOK_ERROR;
                SYNTAX_ERROR("Characters '@' and '`' are reserved")
            case '"':
                FORCE_NEXT;
                ctok->kind = QU_TOK_DOUBLESTRING;
                CONSUME_UNTIL(CHAR == '"' && PREV_CHAR != '\\');
                (void)NEXT;
                break;
            case '\'':
                FORCE_NEXT;
                ctok->kind = QU_TOK_SINGLESTRING;
                CONSUME_UNTIL(CHAR == '\'');
                (void)NEXT;
                break;
            case '#':
                ctok->kind = QU_TOK_COMMENT;
                CONSUME_UNTIL(CHAR == '\n');
                break;
            case '&':
                ctok->kind = QU_TOK_ANCHOR;
                CONSUME_WHILE(FLAGS & CHAR_PLAIN_FLOW);
                break;
            case '*':
                ctok->kind = QU_TOK_ALIAS;
                CONSUME_WHILE(FLAGS & CHAR_PLAIN_FLOW);
                break;
            case '!':
                ctok->kind = QU_TOK_TAG;
                FORCE_NEXT;
                if(CHAR == '!') FORCE_NEXT;
                if(CHAR == '<') {
                    FORCE_NEXT;
                    LOOP {
                        if(CHAR == '>') {
                            (void)NEXT;
                            break;
                        } else if(!(FLAGS & CHAR_URI)) {
                            SYNTAX_ERROR("Wrong char in tag URI");
                        }
                    }
                } else {
                    CONSUME_WHILE(FLAGS & CHAR_TAG);
                }
                break;
            case '|':
                ctok->kind = QU_TOK_LITERAL;
                goto blockscalar;
            case '>':
                ctok->kind = QU_TOK_FOLDED;
            blockscalar: {
                FORCE_NEXT;
                int indent = ctx->indent+1;
                if(CHAR == '-' || CHAR == '+') { // chop
                    FORCE_NEXT;
                }
                if(KLASS == CHAR_DIGIT) {
                    unsigned char *endptr;
                    indent = strtol((char *)ctx->ptr, (char **)&endptr, 10);
                    if(indent > 10000 || indent <= 0)
                        SYNTAX_ERROR("Wrong indentation indicator");
                    indent += ctx->indent;
                    ctx->curpos += endptr - ctx->ptr;
                    ctx->ptr = endptr;
                }
                if(CHAR == '-' || CHAR == '+') { // chop
                    FORCE_NEXT;
                }
                CONSUME_WHILE(KLASS == CHAR_WHITESPACE && CHAR != '\n');
                if(CHAR == '#') {
                    CONSUME_WHILE(CHAR != '\n');
                }
                if(CHAR != '\n')
                    SYNTAX_ERROR("Garbage after block scalar indicator");
                ctx->curline += 1;
                ctx->curpos = 0;
                if(!NEXT) break;
                for(;;) {
                    unsigned char *linestart = ctx->ptr;
                    CONSUME_WHILE(CHAR == ' ');
                    if(ctx->curpos-1 < indent && CHAR != '\n') {
                        ctx->ptr = linestart;
                        ctx->curpos = 1;
                        ctx->linestart = 1;
                        ctx->indent = 0;
                        break;
                    } else {
                    }
                    CONSUME_WHILE(CHAR != '\n');
                    ctx->curline += 1;
                    ctx->curpos = 0;
                    if(!NEXT) break;
                }
                } break;
            // flow indicators
            case ',':
                ctok->kind = QU_TOK_FLOW_ENTRY;
                (void)NEXT;
                break;
            case '[':
                ctok->kind = QU_TOK_FLOW_SEQ_START;
                if(ctx->flow_num >= MAX_FLOW_STACK)
                    SYNTAX_ERROR("Too big flow context depth");
                ctx->flow_stack[ctx->flow_num ++] = '[';
                (void)NEXT;
                break;
            case ']':
                ctok->kind = QU_TOK_FLOW_SEQ_END;
                if(ctx->flow_num == 0 ||
                   ctx->flow_stack[-- ctx->flow_num] != '[')
                    SYNTAX_ERROR("Unmatched ]");
                (void)NEXT;
                break;
            case '{':
                ctok->kind = QU_TOK_FLOW_MAP_START;
                if(ctx->flow_num >= MAX_FLOW_STACK)
                    SYNTAX_ERROR("Too big flow context depth");
                ctx->flow_stack[ctx->flow_num ++] = '{';
                (void)NEXT;
                break;
            case '}':
                ctok->kind = QU_TOK_FLOW_MAP_END;
                if(ctx->flow_num == 0 ||
                   ctx->flow_stack[-- ctx->flow_num] != '{')
                    SYNTAX_ERROR("Unmatched }");
                (void)NEXT;
                break;
            // end of indicators
            }
            if(KLASS != CHAR_WHITESPACE && CHAR != '-') {
                ctx->linestart = 0;
            }
            break;
        case CHAR_WHITESPACE:
            ctok->kind = QU_TOK_WHITESPACE;
            LOOP {
                if(*ctx->ptr == '\n') {
                    ctx->curline += 1;
                    ctx->curpos = 0;
                    ctx->linestart = 1;
                }
                if(KLASS != CHAR_WHITESPACE) {
                    break;
                }
            }
            if(ctx->linestart) {
                ctx->indent = ctx->curpos-1;
                ctok->kind = QU_TOK_INDENT;
            }
            break;
        default:
            ctx->linestart = 0;
            if(FLAGS & CHAR_PLAIN) {
                if(CHAR == '.' && NEXT && CHAR == '.' && NEXT && CHAR == '.') {
                    ctok->kind = QU_TOK_DOC_END;
                    (void)NEXT;
                    break;
                }
        plainstring:
                ctok->kind = QU_TOK_PLAINSTRING;
                int flag = ctx->flow_num ? CHAR_PLAIN_FLOW : CHAR_PLAIN;
                LOOP {
                    if(!(FLAGS & flag) && CHAR != ' ' && CHAR != '\t') break;
                    if(CHAR == ':' && NEXT_KLASS == CHAR_WHITESPACE) break;
                    if(KLASS == CHAR_WHITESPACE && NEXT_CHAR == '#') break;
                }
                if(!ctx->flow_num && CHAR == '\n') {
                    /* scalar may continue next line */
                    do {
                        int indent = ctx->scalarindent+1;
                        ctx->curline += 1;
                        ctx->curpos = 0;
                        if(!NEXT) break;
                        for(;;) {
                            unsigned char *linestart = ctx->ptr;
                            CONSUME_WHILE(CHAR == ' ');
                            if(ctx->curpos-1 < indent && CHAR != '\n') {
                                ctx->ptr = linestart;
                                ctx->curpos = 1;
                                ctx->linestart = 1;
                                ctx->indent = 0;
                                break;
                            } else {
                            }
                            CONSUME_WHILE(CHAR != '\n');
                            ctx->curline += 1;
                            ctx->curpos = 0;
                            if(!NEXT) break;
                        }
                    } while(0);
                } else if(CHAR != '\n') {
                    ctx->scalarindent = ctx->indent;
                }
                // Let's strip trailing whitespace
                while(PREV_CHAR == ' ' || PREV_CHAR == '\t' || PREV_CHAR == '\n') {
                    if(PREV_CHAR == '\n')
                        ctx->curline -= 1;
                    --ctx->curpos;
                    --ctx->ptr;
                }
            } else {
                SYNTAX_ERROR("Wrong character, only printable chars allowed");
            }
            break;
        }
        assert(ctx->ptr <= end);
        ctok->end_line = ctx->curline;
        ctok->end_char = ctx->curpos-1;
        ctok->bytelen = ctx->ptr - ctok->data;
        if(ctok->kind == QU_TOK_DOC_END)
            break;
    }
    if(CIRCLEQ_LAST(&ctx->tokens)->kind != QU_TOK_DOC_END) {
        qu_token *ctok = init_token(ctx);
        ctok->kind = QU_TOK_DOC_END;
    }

#undef NEXT
#undef SYNTAX_ERROR
}


#define CTOK (ctx->cur_token)
#define NTOK (CIRCLEQ_NEXT(CTOK, lst) == (void *)&ctx->tokens ? NULL : CIRCLEQ_NEXT(CTOK, lst))
#define NEXT { assert(CTOK); CTOK = NTOK; SKIP; }
#define SKIP while(CTOK && (CTOK->kind == QU_TOK_WHITESPACE \
                            || CTOK->kind == QU_TOK_INDENT \
                            || CTOK->kind == QU_TOK_COMMENT)) { \
    CTOK = NTOK; \
    }
#define QU_TOK_SCALAR(tok) ((tok)->kind == QU_TOK_PLAINSTRING || \
                           (tok)->kind == QU_TOK_SINGLESTRING || \
                           (tok)->kind == QU_TOK_DOUBLESTRING || \
                           (tok)->kind == QU_TOK_LITERAL || \
                           (tok)->kind == QU_TOK_FOLDED)

qu_ast_node *parse_flow_node(struct qu_parser *ctx) {
    if(CTOK->kind == QU_TOK_ALIAS) {
        qu_ast_node *target = qu_find_anchor(ctx,
            (char *)CTOK->data+1, CTOK->bytelen-1);
        qu_ast_node *node = qu_new_alias_node(ctx, CTOK, target);
        if(!target) {
            node->val.alias_target = qu_new_text_node(ctx, NULL);
            qu_err_node_error(ctx->err, node, "Anchor \"%.*s\" not found",
                CTOK->bytelen-1, (const char *)CTOK->data+1);
        }
        NEXT;
        return node;
    }
    if(CTOK->kind == QU_TOK_ANCHOR) {
        ctx->cur_anchor = CTOK;
        NEXT;
        if(CTOK->kind == QU_TOK_TAG) {
            ctx->cur_tag = CTOK;
            NEXT;
        }
    } else if(CTOK->kind == QU_TOK_TAG) {
        ctx->cur_tag = CTOK;
        NEXT;
        if(CTOK->kind == QU_TOK_ANCHOR) {
            ctx->cur_anchor = CTOK;
            NEXT;
        }
    }
    if(QU_TOK_SCALAR(CTOK)) {
        qu_ast_node *res = qu_new_text_node(ctx, CTOK);
        NEXT;
        return res;
    }
    if(CTOK->kind == QU_TOK_FLOW_MAP_START) {
        qu_ast_node *node = qu_new_mapping_node(ctx, CTOK);
        NEXT;
        while(CTOK->kind != QU_TOK_FLOW_MAP_END) {
            qu_ast_node *knode = qu_new_text_node(ctx, CTOK);
            const char *cont = qu_node_content(knode);
            if(!cont) {
                qu_err_node_fatal(ctx->err, node,
                    "Only string keys supported");
            }
            NEXT; NEXT;
            qu_ast_node *vnode = parse_flow_node(ctx);
            if(!vnode) {
                qu_err_file_fatal(ctx->err, CTOK->filename, CTOK->start_line,
                    "Unexpected token");
            }
            if(!qu_mapping_add(ctx, node, knode, cont, vnode)) {
                qu_err_node_error(ctx->err, knode,
                    "Duplicate key in mapping");
            }

            if(CTOK->kind == QU_TOK_FLOW_MAP_END)
                break;
            if(CTOK->kind != QU_TOK_FLOW_ENTRY) {
                qu_err_file_fatal(ctx->err, CTOK->filename, CTOK->start_line,
                    "Unexpected token");
            }
            NEXT;
        }
        node->end_token = CTOK;
        NEXT;
        return node;
    }
    if(CTOK->kind == QU_TOK_FLOW_SEQ_START) {
        qu_ast_node *node = qu_new_sequence_node(ctx, CTOK);
        NEXT;
        while(CTOK->kind != QU_TOK_FLOW_SEQ_END) {
            qu_ast_node *child = parse_flow_node(ctx);
            qu_sequence_add(ctx, node, child);
            if(CTOK->kind == QU_TOK_FLOW_SEQ_END)
                break;
            if(CTOK->kind != QU_TOK_FLOW_ENTRY) {
                qu_err_file_fatal(ctx->err, CTOK->filename, CTOK->start_line,
                    "Unexpected token");
            }
            NEXT;
        }
        node->end_token = CTOK;
        NEXT;
        return node;
    }
    if(CTOK->kind == QU_TOK_DOC_END) {
        return qu_new_text_node(ctx, NULL);
    }
    qu_err_file_fatal(ctx->err, CTOK->filename, CTOK->start_line,
        "Unknown syntax");
}

qu_ast_node *parse_node(struct qu_parser *ctx, int current_indent) {
    (void) current_indent;
    //printf("PARSE_NODE "); print_token(CTOK, stdout);
    if(CTOK->kind == QU_TOK_ALIAS) {
        qu_ast_node *target = qu_find_anchor(ctx,
            (char *)CTOK->data+1, CTOK->bytelen-1);
        qu_ast_node *node = qu_new_alias_node(ctx, CTOK, target);
        if(!target) {
            node->val.alias_target = qu_new_text_node(ctx, NULL);
            qu_err_node_error(ctx->err, node, "Anchor \"%.*s\" not found",
                CTOK->bytelen-1, (const char *)CTOK->data+1);
        }
        NEXT;
        return node;
    }
    if(CTOK->kind == QU_TOK_ANCHOR) {
        ctx->cur_anchor = CTOK;
        NEXT;
        if(CTOK->kind == QU_TOK_TAG) {
            ctx->cur_tag = CTOK;
            NEXT;
        }
    } else if(CTOK->kind == QU_TOK_TAG) {
        ctx->cur_tag = CTOK;
        NEXT;
        if(CTOK->kind == QU_TOK_ANCHOR) {
            ctx->cur_anchor = CTOK;
            NEXT;
        }
    }
    if(QU_TOK_SCALAR(CTOK) || CTOK->kind == QU_TOK_MAPPING_KEY) {
        if(CTOK->kind == QU_TOK_MAPPING_KEY ||
            (NTOK && NTOK->kind == QU_TOK_MAPPING_VALUE
            && NTOK->start_line == CTOK->start_line)) {
            // printf("STARTING MAP "); print_token(CTOK, stdout);
            if(CTOK->indent <= ctx->cur_mapping)
                return qu_new_text_node(ctx, NULL);  // null node
            // printf("OK\n");
            // MAPPING
            qu_ast_node *node = qu_new_mapping_node(ctx, CTOK);
            int mapping_indent = CTOK->indent;
            int oldmap = ctx->cur_mapping;
            ctx->cur_mapping = CTOK->indent;
            while(CTOK && CTOK->indent == mapping_indent && (
                (CTOK->kind == QU_TOK_MAPPING_KEY)
                ||
                (QU_TOK_SCALAR(CTOK)
                && NTOK->kind == QU_TOK_MAPPING_VALUE
                && NTOK->start_line == CTOK->start_line)))
            {
                int exkey = CTOK->kind == QU_TOK_MAPPING_KEY;
                if(exkey) {
                    NEXT;
                }
                // printf("MAP KEY "); print_token(CTOK, stdout);
                qu_ast_node *knode = qu_new_text_node(ctx, CTOK);
                const char *cont = qu_node_content(knode);
                if(!cont) {  // duplicate key check
                    qu_err_node_fatal(ctx->err, node,
                        "Only string keys supported");
                }
                qu_ast_node *vnode;
                NEXT;
                if(CTOK->kind == QU_TOK_MAPPING_VALUE) {
                    NEXT;
                    vnode = parse_node(ctx, mapping_indent);
                } else {
                    if(!exkey) {
                        qu_err_node_fatal(ctx->err, node, "Expected colon");
                    }
                    vnode = qu_new_text_node(ctx, NULL);
                }
                assert(vnode);
                if(!qu_mapping_add(ctx, node, knode, cont, vnode)) {
                    qu_err_node_error(ctx->err, knode,
                        "Duplicate key in mapping");
                }
            }
            // printf("END MAP\n");
            ctx->cur_mapping = oldmap;
            node->end_token = CIRCLEQ_PREV(CTOK, lst);
            return node;
        } else { // SCALAR
            qu_ast_node *res = qu_new_text_node(ctx, CTOK);
            NEXT;
            return res;
        }
    }
    if(CTOK->kind == QU_TOK_SEQUENCE_ENTRY) {
        if(CTOK->indent <= ctx->cur_sequence)
            return qu_new_text_node(ctx, NULL);  // null node
        qu_ast_node *node = qu_new_sequence_node(ctx, CTOK);
        int sequence_indent = CTOK->indent;
        int oldseq = ctx->cur_sequence;
        ctx->cur_sequence = sequence_indent;
        while(CTOK
              && CTOK->kind == QU_TOK_SEQUENCE_ENTRY
              && CTOK->indent == sequence_indent) {
            NEXT;
            qu_ast_node *child = parse_node(ctx, sequence_indent);
            qu_sequence_add(ctx, node, child);
        }
        ctx->cur_sequence = oldseq;
        node->end_token = CIRCLEQ_PREV(CTOK, lst);
        return node;
    }
    return parse_flow_node(ctx);
}


qu_ast_node *_qu_parse(struct qu_parser *ctx) {

    ctx->cur_token = CIRCLEQ_FIRST(&ctx->tokens);
    ctx->cur_anchor = NULL;
    ctx->cur_tag = NULL;
    ctx->cur_sequence = -1;
    ctx->cur_mapping = -1;
    SKIP;
    while(CTOK && CTOK->kind == QU_TOK_DIRECTIVE) {
        // TODO(tailhook) parse directives
        NEXT;
    }
    if(!CTOK) return NULL;
    if(CTOK->kind == QU_TOK_DOC_START) NEXT;
    if(CTOK->kind == QU_TOK_DOC_END) {
        return qu_new_text_node(ctx, NULL);
    }

    qu_ast_node *node = parse_node(ctx, -1);

    assert(!ctx->error_kind);  // long jump is done

    if(CTOK->kind != QU_TOK_DOC_END) {
        qu_err_file_fatal(ctx->err, CTOK->filename, CTOK->start_line,
            "Unexpected token at end of document");
    }
    return node;
}


void qu_print_tokens(struct qu_parser *ctx, FILE *stream) {
    qu_token *tok;
    CIRCLEQ_FOREACH(tok, &ctx->tokens, lst) {
        print_token(tok, stream);
    }
}

void qu_file_parse(struct qu_parser *ctx, const char *filename) {
    ctx->filename = obstack_copy0(&ctx->pieces,
        filename, strlen(filename));
    FILE *file = fopen(filename, "rb");
    if(!file)
        qu_err_system_fatal(ctx->err, errno, "Can't open \"%s\"", filename);
    qu_load_stream(ctx, file, filename);
    fclose(file);
    qu_tokenize(ctx);
    ctx->document = _qu_parse(ctx);
    assert (ctx->cur_anchor == NULL);
    assert (ctx->cur_tag == NULL);
    assert (ctx->document != NULL);
}

void qu_stream_parse(struct qu_parser *ctx, const char *filename, FILE *stream)
{
    ctx->filename = obstack_copy0(&ctx->pieces,
        filename, strlen(filename));
    qu_load_stream(ctx, stream, filename);
    fclose(stream);
    qu_tokenize(ctx);
    ctx->document = _qu_parse(ctx);
    assert (ctx->cur_anchor == NULL);
    assert (ctx->cur_tag == NULL);
    assert (ctx->document != NULL);
}

qu_ast_node *qu_file_newparse(struct qu_parser *ctx, const char *filename) {
    _qu_context_reinit(ctx);
    ctx->filename = obstack_copy0(&ctx->pieces,
        filename, strlen(filename));
    FILE *file = fopen(filename, "rb");
    if(!file) {
        qu_err_system_error(ctx->err, errno, "Can't open \"%s\"", filename);
        return NULL;
    }
    qu_load_stream(ctx, file, filename);
    fclose(file);
    qu_tokenize(ctx);
    return _qu_parse(ctx);
}
