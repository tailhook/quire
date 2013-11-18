#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include "eval.h"
#include "vars.h"
#include "../error.h"
#include "../util/parse.h"
#include "../yaml/access.h"
#include "../yaml/parser.h"
#include "../quire_int.h"


typedef enum {
    VAR_INT,
    VAR_STRING,
    MAX_VARTYPE
} qu_vartype_t;

struct qu_eval_var {
    qu_vartype_t type;
    union {
        long intvalue;
        struct {
            const char *value;
            int length;
        } str;
    } data;
};

typedef enum {
    TOK_NONE,
    TOK_END,
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_IDENT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_PRODUCT,
    TOK_DIVISION,
    TOK_MODULO,
    TOK_LPAREN,
    TOK_RPAREN,
    MAX_TOK
} token_t;

struct qu_eval_ctx {
    struct qu_parser *parser;
    struct qu_errbuf *err;
    qu_ast_node *node;
    struct qu_var_frame *frame;
    struct obstack *buf;

    const char *data;
    const char *token;
    const char *next;
    const char *end;
    token_t curtok;
};

static struct char_token_s {
    char chvalue;
    token_t token;
} char_token[] = {
    {'+', TOK_PLUS},
    {'-', TOK_MINUS},
    {'*', TOK_PRODUCT},
    {'/', TOK_DIVISION},
    {'%', TOK_MODULO},
    {'(', TOK_LPAREN},
    {')', TOK_RPAREN},
    {'}', TOK_END},
    {'\0', TOK_END}  // to feel safer
    };

static token_t _next_tok(const char **data, const char *end) {
    int i;
    const char *cur = *data;
    while(isspace(*cur)) ++cur;
    for(i = 0; i < (int)(sizeof(char_token)/sizeof(char_token[0])); ++i) {
        if(char_token[i].chvalue == *cur) {
            ++cur;
            *data = cur;
            return char_token[i].token;
        }
    }
    if(isalpha(*cur)) {
        while(cur < end && (isalnum(*cur) || *cur == '_')) ++cur;
        *data = cur;
        return TOK_IDENT;
    }
    if(isdigit(*cur)) {
        while(cur < end && (isalnum(*cur) || *cur == '_')) ++cur;
        *data = cur;
        return TOK_INT;
    }
    if(*cur == '"') {
        while(cur < end && (*cur != '"' || cur[-1] == '\\')) ++cur;
        ++cur;  // set position after the quote
        *data = cur;
        return TOK_STRING;
    }
    return TOK_NONE;
}

static void next_tok(struct qu_eval_ctx *ctx) {
    ctx->token = ctx->next;
    while(isspace(*ctx->token)) ++ctx->token;
    if(ctx->token == ctx->end) {
        ctx->curtok = TOK_END;
    } else {
        ctx->curtok = _next_tok(&ctx->next, ctx->end);
    }
}

static void qu_eval_intern(struct qu_eval_ctx *ctx, const char *data);

static void qu_var_to_int(struct qu_eval_ctx *ctx, struct qu_eval_var *var) {
    long value;
    switch(var->type) {
    case VAR_INT:
        break;
    case VAR_STRING:
        if(!qu_parse_int(var->data.str.value, &value)) {
            qu_err_node_warn(ctx->err, ctx->node, "Bad integer value");
        }
        var->type = VAR_INT;
        var->data.intvalue = value;
        break;
    default:
        qu_err_node_fatal(ctx->err, ctx->node, "Undefined var type");
    }
}

static void qu_append_var_to(struct qu_eval_var *var, struct obstack *buf) {
    int len;
    char intbuf[24];  // should be enought for long

    switch(var->type) {
    case VAR_STRING:
        obstack_grow(buf, var->data.str.value, var->data.str.length);
        break;
    case VAR_INT:
        len = sprintf(intbuf, "%ld", var->data.intvalue);
        if(len <= 0)  /* TODO(tailhook) report this */
            return;
        obstack_grow(buf, intbuf, len);
        break;
    default:
        break;
    }
}

static void eval_sum(struct qu_eval_ctx *ctx, struct qu_eval_var *var);

static void eval_atom(struct qu_eval_ctx *ctx, struct qu_eval_var *res) {
    next_tok(ctx);
    if(ctx->curtok == TOK_LPAREN) {
        eval_sum(ctx, res);
        if(ctx->curtok != TOK_RPAREN) {
            qu_err_node_error(ctx->err, ctx->node,
                "Unclosed parenthesis");
        }
        next_tok(ctx);
    } else if(ctx->curtok == TOK_INT) {
        res->type = VAR_INT;
        res->data.intvalue = atoi(ctx->token);  /* TODO(tailhook) fix */
        next_tok(ctx);
    } else if(ctx->curtok == TOK_STRING) {
        res->type = VAR_STRING;
        res->data.str.value = ctx->token+1;
        res->data.str.length = ctx->next - ctx->token -1;
        next_tok(ctx);
    } else if(ctx->curtok == TOK_IDENT) {
        res->type = VAR_STRING;
        if(!qu_string_var(ctx->frame, ctx->token, ctx->next - ctx->token,
                &res->data.str.value, &res->data.str.length)
            && !qu_anchor_var(ctx->parser, ctx->token, ctx->next - ctx->token,
                &res->data.str.value, &res->data.str.length))
        {
            qu_err_node_warn(ctx->err, ctx->node,
                "Undefined variable \"%.*s\'",
                (int)(ctx->next - ctx->token), ctx->token);
            res->data.str.value = "";
            res->data.str.length = 0;
        }
        next_tok(ctx);
    } else {
        qu_err_node_warn(ctx->err, ctx->node,
            "unexpected token \"%.*s\'",
            (int)(ctx->next - ctx->token), ctx->token);
        res->type = VAR_STRING;
        res->data.str.value = "";
        res->data.str.length = 0;
    }
}

static void eval_product(struct qu_eval_ctx *ctx, struct qu_eval_var *res) {
    eval_atom(ctx, res);
    while(ctx->curtok == TOK_PRODUCT
        || ctx->curtok == TOK_DIVISION
        || ctx->curtok == TOK_MODULO) {
        token_t op = ctx->curtok;
        struct qu_eval_var right;
        eval_atom(ctx, &right);
        qu_var_to_int(ctx, res);
        qu_var_to_int(ctx, &right);
        if(op == TOK_PRODUCT) {
            res->data.intvalue *= right.data.intvalue;
        }
        if(op == TOK_DIVISION) {
            res->data.intvalue /= right.data.intvalue;
        }
        if(op == TOK_MODULO) {
            res->data.intvalue %= right.data.intvalue;
        }
    }
}

static void eval_sum(struct qu_eval_ctx *ctx, struct qu_eval_var *res) {
    eval_product(ctx, res);
    while(ctx->curtok == TOK_PLUS || ctx->curtok == TOK_MINUS) {
        token_t op = ctx->curtok;
        struct qu_eval_var right;
        eval_product(ctx, &right);
        qu_var_to_int(ctx, res);
        qu_var_to_int(ctx, &right);
        if(op == TOK_PLUS) {
            res->data.intvalue += right.data.intvalue;
        }
        if(op == TOK_MINUS) {
            res->data.intvalue -= right.data.intvalue;
        }
    }
}

static void qu_evaluate(struct qu_eval_ctx *ctx, struct qu_eval_var *var,
    const char *data, int dlen)
{
    ctx->data = data;
    ctx->end = data+dlen;
    ctx->token = data;
    ctx->next = data;
    ctx->curtok = TOK_NONE;

    eval_sum(ctx, var);
    if(ctx->token != ctx->end || ctx->curtok != TOK_END) {
        qu_err_node_error(ctx->err, ctx->node,
            "Garbage at the end of expression");
    }
}

static void qu_eval_intern(struct qu_eval_ctx *ctx, const char *data) {
    const char *c;
    for(c = data; *c;) {
        if(*c != '$' && *c != '\\') {
            obstack_1grow(ctx->buf, *c);
            ++c;
            continue;
        }
        if(*c == '\\') {
            if(!*++c) {
                qu_err_node_error(ctx->err, ctx->node,
                    "Expected char after backslash");
            }
            obstack_1grow(ctx->buf, *c);
            ++c;
            continue;
        }
        ++c;
        const char *name = c;
        int nlen;
        if(*c == '{') {
            ++c;
            ++name;
            while(*++c && *c != '}');
            nlen = c - name;
            if(*c++ != '}') {
                qu_err_node_error(ctx->err, ctx->node,
                    "Expected closing }");
            }
            struct qu_eval_var var;
            qu_evaluate(ctx, &var, name, nlen);
            qu_append_var_to(&var, ctx->buf);
        } else {
            while(*c && (isalnum(*c) || *c == '_')) ++c;
            nlen = c - name;
            if(nlen == 0) {
                obstack_1grow(ctx->buf, '$');
                continue;
            }
            const char *value;
            int value_len;
            if(qu_string_var(ctx->frame, name, nlen, &value, &value_len)) {
                obstack_grow(ctx->buf, value, value_len);
            } else if(qu_anchor_var(ctx->parser, name, nlen,
                                    &value, &value_len)) {
                obstack_grow(ctx->buf, value, value_len);
            }
        }
    }
}

void qu_eval_str(struct qu_parser *parser, struct qu_var_frame *vars,
    const char *data, qu_ast_node *node,
    const char **result, int *rlen)
{
    struct qu_eval_ctx eval = {
        .parser = parser,
        .err = parser->err,
        .node = node,
        .frame = vars,
        .buf = &parser->pieces,
        };
    if(strchr(data, '$')) {
        obstack_blank(eval.buf, 0);

        qu_eval_intern(&eval, data);

        obstack_1grow(eval.buf, 0);
        *rlen = obstack_object_size(eval.buf)-1;
        *result = obstack_finish(eval.buf);
    } else {
        *rlen = strlen(data);
        *result = obstack_copy0(eval.buf, data, *rlen);
    }
}


