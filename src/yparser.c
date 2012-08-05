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

#include "yparser.h"

char c_sequence_entry[] = "-";
char c_mapping_key[] = "?";
char c_mapping_value[] = ":";
char c_collect_entry[] = ",";
char c_sequence_start[] = "[";
char c_sequence_end[] = "]";
char c_mapping_start[] = "{";
char c_mapping_end[] = "}";
char c_comment[] = "#";
char c_anchor[] = "&";
char c_alias[] = "*";
char c_tag[] = "!";
char c_literal[] = "|";
char c_folded[] = ">";
char c_single_quote[] = "'";
char c_double_quote[] = "\"";
char c_directive[] = "%";
char c_reserved[] = "@`";
char c_indicator[] = "-?:,[]{}#&*!|>'\"%@`";
char c_flow_indicator[] = ",[]{}";
char b_line_feed[] = "\n";
char b_carriage_return[] = "\r";
char b_char[] = "\n\r";
char c_printable[] = "\t\r\n !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
char nb_char[] = "\t !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
// nb_char = c-printable - b-char
// b_break = CRLF | CR | LF
char s_space[] = " ";
char s_tab[] = "\t";
char s_white[] = " \t";
char ns_char[] = "!\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
char ns_plain_flow_char[] = "!\"#$%&\'()*+-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\^_`abcdefghijklmnopqrstuvwxyz|~";
// ns_char = nb_char - s_white
char ns_dec_digit[] = "0123456789";
char ns_hex_digit[] = "0123456789ABCDEFabcdef";
char ns_ascii_letter[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
char ns_word_char[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-";
char ns_uri_char[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-#;/?:@&=+$,_.!~*'()[]";
char ns_tag_char[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-#;/?:@&=+$_.~*'()";

typedef struct {
    char klass;
    char flags;
} charinfo;

charinfo chars[256];

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


typedef enum yaml_error {
    YAML_NO_ERROR,
    YAML_SCANNER_ERROR,
    YAML_PARSER_ERROR,
    YAML_CONTENT_ERROR
} yaml_error;

typedef enum char_klass {
    CHAR_UNKNOWN,
    CHAR_INDICATOR,
    CHAR_WHITESPACE,
    CHAR_DIGIT,
} char_klass;

typedef enum char_flag {
    CHAR_PRINTABLE=1,
    CHAR_FLOW_INDICATOR=2,
    CHAR_URI=4,
    CHAR_TAG=8,
    CHAR_PLAIN=16,
    CHAR_PLAIN_FLOW=32,
} char_flag;

static void safeprint(FILE *stream, unsigned char *data, int len) {
    for(unsigned char *c = data, *end = data + len; c < end; ++c) {
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

static void print_token(yaml_token *tok, FILE *stream) {
    fprintf(stream, "%s:%d %s [%d]``", tok->filename, tok->start_line,
        token_to_str[tok->kind], tok->indent);
    safeprint(stream, tok->data, tok->bytelen);
    fprintf(stream, "''\n");
}

void yaml_init() {
    chars['\t'].flags |= CHAR_PRINTABLE;
    chars['\r'].flags |= CHAR_PRINTABLE;
    chars['\n'].flags |= CHAR_PRINTABLE;
    for(int i = 0x20; i <= 0x7E; ++i)
        chars[i].flags |= CHAR_PRINTABLE;
    for(int i = 0x80; i <= 0xFF; ++i)  // UTF-8 encoded chars
        chars[i].flags |= CHAR_PRINTABLE|CHAR_PLAIN;
    for(int i = 0; i < sizeof(c_flow_indicator)-1; ++i)
        chars[(int)c_flow_indicator[i]].flags |= CHAR_FLOW_INDICATOR;
    for(int i = 0; i < sizeof(ns_tag_char)-1; ++i)
        chars[(int)ns_tag_char[i]].flags |= CHAR_TAG;
    for(int i = 0; i < sizeof(ns_uri_char)-1; ++i)
        chars[(int)ns_uri_char[i]].flags |= CHAR_URI;
    for(int i = 0; i < sizeof(ns_plain_flow_char)-1; ++i)
        chars[(int)ns_plain_flow_char[i]].flags |= CHAR_PLAIN_FLOW;
    for(int i = 0; i < sizeof(ns_char)-1; ++i)
        chars[(int)ns_char[i]].flags |= CHAR_PLAIN;
    for(int i = 0; i < sizeof(c_indicator)-1; ++i)
        chars[(int)c_indicator[i]].klass = CHAR_INDICATOR;
    chars[' '].klass = CHAR_WHITESPACE;
    chars['\t'].klass = CHAR_WHITESPACE;
    chars['\r'].klass = CHAR_WHITESPACE;
    chars['\n'].klass = CHAR_WHITESPACE;
    for(int i = '0'; i <= '9'; ++i) {
        chars[i].klass = CHAR_DIGIT;
    }
}


void *obstack_chunk_alloc(int size) {
    void *res = malloc(size);
    assert(res);
    // TODO(tailhook) implement longjump here
    return res;
}

void obstack_chunk_free(void *ptr) {
    free(ptr);
}


int yaml_context_init(yaml_parse_context *ctx) {
    obstack_init(&ctx->pieces);
    CIRCLEQ_INIT(&ctx->tokens);
    LIST_INIT(&ctx->anchors);
    ctx->buf = NULL;
    ctx->error_kind = 0;
    return 0;
}

int yaml_load_file(yaml_parse_context *ctx, char *filename) {
    int eno;
    unsigned char *data = NULL;
    int fd = open(filename, O_RDONLY);
    if(fd < 0) return -1;
    struct stat stinfo;
    int rc = fstat(fd, &stinfo);
    if(rc < 0) goto error;
    data = malloc(stinfo.st_size+1);
    if(!data) goto error;
    int so_far = 0;
    while(so_far < stinfo.st_size) {
        rc = read(fd, data + so_far, stinfo.st_size - so_far);
        if(rc == -1) {
            if(errno == EINTR) continue;
            goto error;
        }
        if(!rc) {
            // WARNING: file truncated
            break;
        }
        so_far += rc;
    }
    rc = close(fd);
    if(rc < 0) return -1;
    ctx->filename = obstack_copy0(&ctx->pieces, filename, strlen(filename));
    ctx->buf = data;
    ctx->buf[so_far] = 0;
    ctx->buflen = so_far;
    return 0;

error:
    free(data);
    eno = errno;
    close(fd);
    errno = eno;
    return -1;
}

int yaml_context_free(yaml_parse_context *ctx) {
    obstack_free(&ctx->pieces, NULL);
    if(ctx->buf) {
        free(ctx->buf);
    }
    return 0;
}

static yaml_token *init_token(yaml_parse_context *ctx) {
    yaml_token *ctok = obstack_alloc(&ctx->pieces, sizeof(yaml_token));
    ctok->kind = TOKEN_ERROR;
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

int yaml_tokenize(yaml_parse_context *ctx) {

#define KLASS (chars[(int)CHAR].klass)
#define FLAGS (chars[(int)CHAR].flags)
#define NEXT_KLASS (chars[(int)NEXT_CHAR].klass)
#define NEXT_FLAGS (chars[(int)NEXT_CHAR].flags)
#define LOOP for(; ctx->ptr < end; ++ctx->ptr, ++ctx->curpos)
#define NEXT ((++ctx->curpos, ++ctx->ptr) < end)
#define FORCE_NEXT if((++ctx->curpos, ++ctx->ptr) >= end) { \
    ctok->kind = TOKEN_ERROR; \
    ctok->end_line = ctx->curline; \
    ctok->end_char = ctx->curpos; \
    ctx->error_kind = YAML_SCANNER_ERROR; \
    ctx->error_text = "Premature end of file"; \
    ctx->error_token = ctok; \
    return 0; \
    }
#define SYNTAX_ERROR(message) {\
    ctok->kind = TOKEN_ERROR; \
    ctok->end_line = ctx->curline; \
    ctok->end_char = ctx->curpos; \
    ctx->error_kind = YAML_SCANNER_ERROR; \
    ctx->error_text = message; \
    ctx->error_token = ctok; \
    return 0; \
    }

#define CONSUME_WHILE(cond) LOOP { if(!(cond)) break; }
#define CONSUME_UNTIL(cond) LOOP { if(cond) break; }

#define CHAR (*ctx->ptr)
#define PREV_CHAR (*(ctx->ptr-1))  // use carefully!
#define NEXT_CHAR (*(ctx->ptr+1))  // safe, since we have \0 at the end

    unsigned char *end = ctx->buf + ctx->buflen;
    ctx->curline = 1;
    ctx->linestart = 1;
    ctx->curpos = 1;
    ctx->indent = 0;
    ctx->ptr = ctx->buf;
    ctx->flow_num = 0;
    for(;ctx->ptr < end;) {
        yaml_token *ctok = init_token(ctx);
        switch(KLASS) {
        case CHAR_INDICATOR:
            switch (CHAR) {
            // Indicators
            case '-': // list element, doc start, plainstring
                FORCE_NEXT;
                if(KLASS == CHAR_WHITESPACE) {
                    ctok->kind = TOKEN_SEQUENCE_ENTRY;
                    if(!ctx->linestart) {
                        ctx->error_kind = YAML_SCANNER_ERROR;
                        ctx->error_text = "Sequence entry dash sign is only"
                                          " allowed at the start of line";
                        ctx->error_token = ctok;
                        return 0;
                    }
                    ctx->indent += 1;
                    ctok->indent = ctx->indent;
                    break;
                } else if(CHAR == '-' && NEXT && CHAR == '-') {
                    ctok->kind = TOKEN_DOC_START;
                    (void)NEXT;
                    break;
                } else {
                    goto plainstring;
                }
            case '?': // key, plainstring
                FORCE_NEXT;
                if(KLASS == CHAR_WHITESPACE) {
                    ctok->kind = TOKEN_MAPPING_KEY;
                    break;
                } else {
                    goto plainstring;
                }
            case ':': // value, plainstring
                FORCE_NEXT;
                if(KLASS == CHAR_WHITESPACE) {
                    ctok->kind = TOKEN_MAPPING_VALUE;
                    break;
                } else if(CIRCLEQ_PREV(ctok, lst) &&
                    CIRCLEQ_PREV(ctok, lst)->kind == TOKEN_DOUBLESTRING) {
                    // JSON-like value {"abc":123}
                    ctok->kind = TOKEN_MAPPING_VALUE;
                    break;
                } else {
                    goto plainstring;
                }
            case '%':
                ctok->kind = TOKEN_DIRECTIVE;
                CONSUME_UNTIL(CHAR == '\n');
                break;
            case '@': case '`':
                ctok->end_char += 1;
                ctok->kind = TOKEN_ERROR;
                ctx->error_kind = YAML_SCANNER_ERROR;
                ctx->error_text = "Characters '@' and '`' are reserved";
                ctx->error_token = ctok;
                return 0;
            case '"':
                FORCE_NEXT;
                ctok->kind = TOKEN_DOUBLESTRING;
                CONSUME_UNTIL(CHAR == '"' && PREV_CHAR != '\\');
                (void)NEXT;
                break;
            case '\'':
                FORCE_NEXT;
                ctok->kind = TOKEN_SINGLESTRING;
                CONSUME_UNTIL(CHAR == '\'');
                (void)NEXT;
                break;
            case '#':
                ctok->kind = TOKEN_COMMENT;
                CONSUME_UNTIL(CHAR == '\n');
                break;
            case '&':
                ctok->kind = TOKEN_ANCHOR;
                CONSUME_WHILE(FLAGS & CHAR_PLAIN_FLOW);
                break;
            case '*':
                ctok->kind = TOKEN_ALIAS;
                CONSUME_WHILE(FLAGS & CHAR_PLAIN_FLOW);
                break;
            case '!':
                ctok->kind = TOKEN_TAG;
                FORCE_NEXT;
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
                ctok->kind = TOKEN_LITERAL;
                goto blockscalar;
            case '>':
                ctok->kind = TOKEN_FOLDED;
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
                        ctx->curpos = 0;
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
                ctok->kind = TOKEN_FLOW_ENTRY;
                (void)NEXT;
                break;
            case '[':
                ctok->kind = TOKEN_FLOW_SEQ_START;
                if(ctx->flow_num >= MAX_FLOW_STACK)
                    SYNTAX_ERROR("Too big flow context depth");
                ctx->flow_stack[ctx->flow_num ++] = '[';
                (void)NEXT;
                break;
            case ']':
                ctok->kind = TOKEN_FLOW_SEQ_END;
                if(ctx->flow_num == 0 ||
                   ctx->flow_stack[-- ctx->flow_num] != '[')
                    SYNTAX_ERROR("Unmatched ]");
                (void)NEXT;
                break;
            case '{':
                ctok->kind = TOKEN_FLOW_MAP_START;
                if(ctx->flow_num >= MAX_FLOW_STACK)
                    SYNTAX_ERROR("Too big flow context depth");
                ctx->flow_stack[ctx->flow_num ++] = '{';
                (void)NEXT;
                break;
            case '}':
                ctok->kind = TOKEN_FLOW_MAP_END;
                if(ctx->flow_num == 0 ||
                   ctx->flow_stack[-- ctx->flow_num] != '{')
                    SYNTAX_ERROR("Unmatched }");
                (void)NEXT;
                break;
            // end of indicators
            }
            if(KLASS != CHAR_WHITESPACE) {
                ctx->linestart = 0;
            }
            break;
        case CHAR_WHITESPACE:
            ctok->kind = TOKEN_WHITESPACE;
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
                ctok->kind = TOKEN_INDENT;
            }
            break;
        default:
            ctx->linestart = 0;
            if(FLAGS & CHAR_PLAIN) {
                if(CHAR == '.' && NEXT && CHAR == '.' && NEXT && CHAR == '.') {
                    ctok->kind = TOKEN_DOC_END;
                    (void)NEXT;
                    break;
                }
        plainstring:
                ctok->kind = TOKEN_PLAINSTRING;
                int flag = ctx->flow_num ? CHAR_PLAIN_FLOW : CHAR_PLAIN;
                LOOP {
                    if(!(FLAGS & flag) && CHAR != ' ' && CHAR != '\t') break;
                    if(CHAR == ':' && NEXT_KLASS == CHAR_WHITESPACE) break;
                    if(KLASS == CHAR_WHITESPACE && NEXT_CHAR == '#') break;
                }
            } else {
                printf("CHAR %c %d\n", CHAR, FLAGS);
                SYNTAX_ERROR("Wrong character, only printable chars allowed");
            }
            break;
        }
        assert(ctx->ptr <= end);
        ctok->end_line = ctx->curline;
        ctok->end_char = ctx->curpos-1;
        ctok->bytelen = ctx->ptr - ctok->data;
        if(ctok->kind == TOKEN_DOC_END)
            break;
    }
    if(CIRCLEQ_LAST(&ctx->tokens)->kind != TOKEN_DOC_END) {
        yaml_token *ctok = init_token(ctx);
        ctok->kind = TOKEN_DOC_END;
    }
    return 0;

#undef NEXT
#undef SYNTAX_ERROR
}

static yaml_ast_node *new_node(yaml_parse_context *ctx) {
    yaml_ast_node *node = obstack_alloc(&ctx->pieces, sizeof(yaml_ast_node));
    node->kind = NODE_UNKNOWN;
    node->ctx = ctx;
    node->anchor = ctx->cur_anchor;
    if(ctx->cur_anchor) {
        LIST_INSERT_HEAD(&ctx->anchors, node, anchors);
        ctx->cur_anchor = NULL;
    }
    node->tag = ctx->cur_tag;
    if(ctx->cur_tag)
        ctx->cur_tag = NULL;
    node->target = NULL;
    node->tag = NULL;
    node->start_token = NULL;
    node->end_token = NULL;
    node->content = NULL;
    node->content_len = 0;
    node->tree = NULL;
    node->left = NULL;
    node->right = NULL;
    CIRCLEQ_INIT(&node->children);
    return node;
}

static yaml_ast_node *new_text_node(yaml_parse_context *ctx, yaml_token *tok) {
    yaml_ast_node *node = new_node(ctx);
    node->kind = NODE_SCALAR;
    node->start_token = tok;
    node->end_token = tok;
    // TODO(tailhook) parse content
    return node;
}

char *yaml_node_content(yaml_ast_node *node) {
    if(node->kind != NODE_SCALAR)
        return NULL;
    if(node->content)
        return node->content;
    if(node->start_token == node->end_token
        && node->start_token->kind == TOKEN_PLAINSTRING) {
        node->content = obstack_copy0(&node->ctx->pieces,
            (char *)node->start_token->data,
            node->start_token->bytelen);
        return node->content;
    }
    fprintf(stderr, "Not implemented");
    assert(0);
}

#define CTOK (ctx->cur_token)
#define NTOK (CIRCLEQ_NEXT(CTOK, lst) == (void *)&ctx->tokens ? NULL : CIRCLEQ_NEXT(CTOK, lst))
#define NEXT { assert(CTOK); CTOK = NTOK; SKIP; }
#define SKIP while(CTOK && (CTOK->kind == TOKEN_WHITESPACE \
                            || CTOK->kind == TOKEN_INDENT \
                            || CTOK->kind == TOKEN_COMMENT)) { \
    CTOK = NTOK; \
    }
#define SYNTAX_ERROR(message) { \
    if(!CTOK) { \
        ctok = CIRCLEQ_LAST(&ctx->tokens); \
    } \
    ctx->error_kind = YAML_PARSER_ERROR; \
    ctx->error_text = message; \
    ctx->error_token = CTOK; \
    return NULL; \
    }
#define TOKEN_SCALAR(tok) ((tok)->kind == TOKEN_PLAINSTRING || \
                           (tok)->kind == TOKEN_SINGLESTRING || \
                           (tok)->kind == TOKEN_DOUBLESTRING || \
                           (tok)->kind == TOKEN_LITERAL || \
                           (tok)->kind == TOKEN_FOLDED)

yaml_ast_node *parse_flow_node(yaml_parse_context *ctx) {
    if(CTOK->kind == TOKEN_ALIAS) {
        yaml_ast_node *node = new_node(ctx);
        node->start_token = CTOK;
        node->end_token = CTOK;
        node->kind = NODE_ALIAS;
        yaml_ast_node *target;
        LIST_FOREACH(target, &ctx->anchors, anchors) {
            if(target->anchor->bytelen == CTOK->bytelen
                && !strncmp((char *)target->anchor->data+1,  // "&" char
                            (char *)CTOK->data+1,  // "*" char
                            CTOK->bytelen-1)) {
                node->target = target;
                NEXT;
                return node;
            }
        }
        // TODO(tailhook) put error message here
        return NULL;
    }
    if(CTOK->kind == TOKEN_ANCHOR) {
        ctx->cur_anchor = CTOK;
        NEXT;
        if(CTOK->kind == TOKEN_TAG) {
            ctx->cur_tag = CTOK;
            NEXT;
        }
    } else if(CTOK->kind == TOKEN_TAG) {
        ctx->cur_tag = CTOK;
        NEXT;
        if(CTOK->kind == TOKEN_ANCHOR) {
            ctx->cur_anchor = CTOK;
            NEXT;
        }
    }
    if(TOKEN_SCALAR(CTOK)) {
        yaml_ast_node *res = new_text_node(ctx, CTOK);
        NEXT;
        return res;
    }
    if(CTOK->kind == TOKEN_FLOW_MAP_START) {
        yaml_ast_node *node = new_node(ctx);
        node->start_token = CTOK;
        node->kind = NODE_MAPPING;
        NEXT;
        while(1) {
            yaml_ast_node *child = new_text_node(ctx, CTOK);
            CIRCLEQ_INSERT_TAIL(&node->children, child, lst);
            NEXT; NEXT;
            child = parse_flow_node(ctx);
            if(!child)
                return NULL;
            CIRCLEQ_INSERT_TAIL(&node->children, child, lst);
            if(CTOK->kind == TOKEN_FLOW_MAP_END)
                break;
            if(CTOK->kind != TOKEN_FLOW_ENTRY)
                return NULL;
            NEXT;
        }
        node->end_token = CTOK;
        NEXT;
        return node;
    }
    if(CTOK->kind == TOKEN_FLOW_SEQ_START) {
        yaml_ast_node *node = new_node(ctx);
        node->kind = NODE_SEQUENCE;
        node->start_token = CTOK;
        NEXT;
        while(1) {
            yaml_ast_node *child = parse_flow_node(ctx);
            CIRCLEQ_INSERT_TAIL(&node->children, child, lst);
            if(CTOK->kind == TOKEN_FLOW_SEQ_END)
                break;
            if(CTOK->kind != TOKEN_FLOW_ENTRY)
                return NULL;
            NEXT;
        }
        node->end_token = CTOK;
        NEXT;
        return node;
    }
    return NULL;
}

yaml_ast_node **find_node(yaml_ast_node **root, char *value) {
    while(*root) {
        int cmp = strcmp(yaml_node_content(*root), value);
        if(!cmp)
            return root;
        if(cmp < 0)
            root = &(*root)->left;
        if(cmp > 0)
            root = &(*root)->right;
    }
    return root;
}

yaml_ast_node *parse_node(yaml_parse_context *ctx) {
    if(CTOK->kind == TOKEN_ALIAS) {
        yaml_ast_node *node = new_node(ctx);
        node->start_token = CTOK;
        node->end_token = CTOK;
        node->kind = NODE_ALIAS;
        yaml_ast_node *target;
        LIST_FOREACH(target, &ctx->anchors, anchors) {
            if(target->anchor->bytelen == CTOK->bytelen
                && !strncmp((char *)target->anchor->data+1,  // "&" char
                            (char *)CTOK->data+1,  // "*" char
                            CTOK->bytelen-1)) {
                node->target = target;
                NEXT;
                return node;
            }
        }
        // TODO(tailhook) put error message here
        return NULL;
    }
    if(CTOK->kind == TOKEN_ANCHOR) {
        ctx->cur_anchor = CTOK;
        NEXT;
        if(CTOK->kind == TOKEN_TAG) {
            ctx->cur_tag = CTOK;
            NEXT;
        }
    } else if(CTOK->kind == TOKEN_TAG) {
        ctx->cur_tag = CTOK;
        NEXT;
        if(CTOK->kind == TOKEN_ANCHOR) {
            ctx->cur_anchor = CTOK;
            NEXT;
        }
    }
    if(TOKEN_SCALAR(CTOK)) {
        if(NTOK
            && NTOK->kind == TOKEN_MAPPING_VALUE
            && NTOK->start_line == CTOK->start_line) {
            // MAPPING
            yaml_ast_node *node = new_node(ctx);
            node->start_token = CTOK;
            node->kind = NODE_MAPPING;
            int mapping_indent = CTOK->indent;
            while(CTOK && TOKEN_SCALAR(CTOK)
                && NTOK->kind == TOKEN_MAPPING_VALUE
                && NTOK->start_line == CTOK->start_line
                && CTOK->indent == mapping_indent) {
                yaml_ast_node *child = new_text_node(ctx, CTOK);
                char *cont = yaml_node_content(child);
                if(cont) {
                    yaml_ast_node **targ = find_node(&node->tree, cont);
                    if(*targ) {
                        obstack_blank(&ctx->pieces, 0);
                        obstack_grow(&ctx->pieces, "Duplicate key \"", 15);
                        obstack_grow(&ctx->pieces, cont, strlen(cont));
                        obstack_grow0(&ctx->pieces, "\"", 1);
                        ctx->error_text = obstack_finish(&ctx->pieces);
                        ctx->error_token = child->start_token;
                        ctx->error_kind = YAML_CONTENT_ERROR;
                        return NULL;
                    } else {
                        *targ = child;
                    }
                }
                CIRCLEQ_INSERT_TAIL(&node->children, child, lst);
                NEXT; NEXT;
                child = parse_node(ctx);
                if(!child)
                    return NULL;
                CIRCLEQ_INSERT_TAIL(&node->children, child, lst);
            }
            node->end_token = CIRCLEQ_PREV(CTOK, lst);
            return node;
        } else { // SCALAR
            yaml_ast_node *res = new_text_node(ctx, CTOK);
            NEXT;
            return res;
        }
    }
    if(CTOK->kind == TOKEN_SEQUENCE_ENTRY) {
        yaml_ast_node *node = new_node(ctx);
        node->kind = NODE_SEQUENCE;
        node->start_token = CTOK;
        int sequence_indent = CTOK->indent;
        while(CTOK
              && CTOK->kind == TOKEN_SEQUENCE_ENTRY
              && CTOK->indent == sequence_indent) {
            NEXT;
            yaml_ast_node *child = parse_node(ctx);
            CIRCLEQ_INSERT_TAIL(&node->children, child, lst);
        }
        node->end_token = CIRCLEQ_PREV(CTOK, lst);
        return node;
    }
    return parse_flow_node(ctx);
}


int yaml_parse(yaml_parse_context *ctx) {

    ctx->cur_token = CIRCLEQ_FIRST(&ctx->tokens);
    ctx->cur_anchor = NULL;
    ctx->cur_tag = NULL;
    SKIP;
    while(CTOK && CTOK->kind == TOKEN_DIRECTIVE) {
        // TODO(tailhook) parse directives
        NEXT;
    }
    if(!CTOK) return 0;
    if(CTOK->kind == TOKEN_DOC_START) NEXT;
    if(CTOK->kind == TOKEN_DOC_END) return 0;

    ctx->document = parse_node(ctx);

    if(ctx->error_kind)
        return 0;

    if(CTOK->kind != TOKEN_DOC_END) {
        print_token(CTOK, stdout);
        assert(0);
    }

    return 0;
}


void yaml_print_tokens(yaml_parse_context *ctx, FILE *stream) {
    yaml_token *tok;
    CIRCLEQ_FOREACH(tok, &ctx->tokens, lst) {
        print_token(tok, stream);
    }
}


