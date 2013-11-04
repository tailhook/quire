#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include "eval.h"
#include "vars.h"
#include "context.h"
#include "../util/parse.h"
#include "../yaml/access.h"
#include "../quire_int.h"


typedef enum {
    VAR_INT,
    VAR_STRING,
    MAX_VARTYPE
} vartype_t;

typedef struct variable_s {
    vartype_t type;
    union {
        long intvalue;
        struct {
            const char *value;
            int length;
        } str;
    } data;
} variable_t;

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
    struct qu_config_context *ctx;
    struct obstack *buf;
    qu_ast_node *node;
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
        while(cur < end && isalnum(*cur)) ++cur;
        *data = cur;
        return TOK_IDENT;
    }
    if(isdigit(*cur)) {
        while(cur < end && isalnum(*cur)) ++cur;
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

static void qu_var_to_int(struct qu_eval_ctx *ctx, variable_t *var) {
    const char *end;
    long value;
    switch(var->type) {
    case VAR_INT:
        break;
    case VAR_STRING:
        end = qu_parse_int(var->data.str.value, &value);
        if(end != var->data.str.value + var->data.str.length) {
            qu_report_error(&ctx->ctx->parser, ctx->node, "Bad integer value");
        }
        var->type = VAR_INT;
        var->data.intvalue = value;
        break;
    default:
        qu_report_error(&ctx->ctx->parser, ctx->node, "Undefined var type");
    }
}

static void qu_append_var_to(variable_t *var, struct obstack *buf) {
    int len;
    char intbuf[24];  // should be enought for long

    if(!var)
        return;
    switch(var->type) {
    case VAR_STRING:
        obstack_grow(buf, var->data.str.value, var->data.str.length);
        break;
    case VAR_INT:
        len = sprintf(intbuf, "%ld", var->data.intvalue);
        if(len <= 0)
            return;
        obstack_grow(buf, intbuf, len);
        break;
    default:
        break;
    }
}

static variable_t *eval_sum(struct qu_eval_ctx *ctx);

static variable_t *eval_atom(struct qu_eval_ctx *ctx) {
    variable_t *res;
    next_tok(ctx);
    if(ctx->curtok == TOK_LPAREN) {
        variable_t *res = eval_sum(ctx);
        if(ctx->curtok != TOK_RPAREN) {
            qu_report_error(&ctx->ctx->parser, ctx->node,
                "Unclosed parenthesis");
        }
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_INT) {
        res = obstack_alloc(ctx->buf, sizeof(variable_t));
        res->type = VAR_INT;
        res->data.intvalue = atoi(ctx->token);
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_STRING) {
        res = obstack_alloc(ctx->buf,
            sizeof(variable_t) + ctx->next - ctx->token - 1);
        res->type = VAR_STRING;
        char *data = (char *)res + sizeof(variable_t);
        memcpy(data, ctx->token+1, ctx->next - ctx->token - 1);
        data[ctx->next - ctx->token - 2] = 0;
        res->data.str.value = data;
        res->data.str.length = ctx->next - ctx->token;
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_IDENT) {
        res = obstack_alloc(ctx->buf, sizeof(variable_t));
        res->type = VAR_STRING;
        if(qu_get_string_len(ctx->ctx, ctx->token, ctx->next - ctx->token,
            &res->data.str.value, &res->data.str.length) < 0) {
            res->data.str.value = "";
            res->data.str.length = 0;
        }
        next_tok(ctx);
        return res;
    } else {
        qu_report_error(&ctx->ctx->parser, ctx->node, "Bad token");
        abort();
    }
}

static variable_t *eval_product(struct qu_eval_ctx *ctx) {
    variable_t *left = eval_atom(ctx);
    while(ctx->curtok == TOK_PRODUCT
        || ctx->curtok == TOK_DIVISION
        || ctx->curtok == TOK_MODULO) {
        token_t op = ctx->curtok;
        variable_t *right = eval_product(ctx);
        qu_var_to_int(ctx, left);
        qu_var_to_int(ctx, right);
        if(op == TOK_PRODUCT) {
            left->data.intvalue *= right->data.intvalue;
        }
        if(op == TOK_DIVISION) {
            left->data.intvalue /= right->data.intvalue;
        }
        if(op == TOK_MODULO) {
            left->data.intvalue %= right->data.intvalue;
        }
    }
    return left;
}

static variable_t *eval_sum(struct qu_eval_ctx *ctx) {
    variable_t *left = eval_product(ctx);
    while(ctx->curtok == TOK_PLUS || ctx->curtok == TOK_MINUS) {
        token_t op = ctx->curtok;
        variable_t *right = eval_product(ctx);
        qu_var_to_int(ctx, left);
        qu_var_to_int(ctx, right);
        if(op == TOK_PLUS) {
            left->data.intvalue += right->data.intvalue;
        }
        if(op == TOK_MINUS) {
            left->data.intvalue -= right->data.intvalue;
        }
    }
    return left;
}

static variable_t *qu_evaluate(struct qu_config_context *ctx,
    const char *data, int dlen, qu_ast_node *node)
{
    struct qu_eval_ctx eval = {
        .ctx = ctx,
        .buf = &ctx->parser.pieces,
        .node = node,
        .data = data,
        .end = data+dlen,
        .token = data,
        .next = data,
        .curtok = TOK_NONE
        };
    variable_t *res = eval_sum(&eval);
    if(eval.token != eval.end || eval.curtok != TOK_END) {
        qu_report_error(&ctx->parser, node,
            "Garbage at the end of expression");
    }
    return res;
}

void qu_eval_str(qu_config_context *info,
    const char *data, int interp, qu_ast_node *node,
    const char **result, int *rlen);

void qu_eval_int(qu_config_context *info, const char *value,
    int interp, qu_ast_node *node, long *result)
{
    if(interp && strchr(value, '$')) {
        const char *data;
        int dlen;
        qu_eval_str(info, value, interp, node, &data, &dlen);
        const char *end = qu_parse_int(data, result);
        obstack_free(info->alloc, (void *)data);
        if(end != data + dlen) {
            LONGJUMP_WITH_CONTENT_ERROR(&info->parser, info->parser.cur_token,
                "Integer value required");
        }

        return;
    }
    const char *end = qu_parse_int(value, result);
    int vlen = strlen(value);
    if(end != value + vlen) {
        LONGJUMP_WITH_CONTENT_ERROR(&info->parser, info->parser.cur_token,
            "Integer value required");
    }
}

void qu_eval_bool(qu_config_context *info, const char *value,
    int interp, qu_ast_node *node, int *result)
{
    if(interp && strchr(value, '$')) {
        const char *data;
        int dlen;
        qu_eval_str(info, value, interp, node, &data, &dlen);
        int ok = qu_parse_bool(data, result);
        obstack_free(info->alloc, (void *)data);
        if(!ok) {
            LONGJUMP_WITH_CONTENT_ERROR(&info->parser, info->parser.cur_token,
                "Integer value required");
        }

        return;
    }
    if(!qu_parse_bool(value, result)) {
        LONGJUMP_WITH_CONTENT_ERROR(&info->parser, info->parser.cur_token,
            "Integer value required");
    }
}

void qu_eval_float(qu_config_context *info, const char *value,
    int interp, qu_ast_node *node, double *result)
{
    if(interp && strchr(value, '$')) {
        const char *data;
        int dlen;
        qu_eval_str(info, value, interp, node, &data, &dlen);
        const char *end = qu_parse_float(data, result);
        obstack_free(info->alloc, (void *)data);
        if(end != data + dlen) {
            LONGJUMP_WITH_CONTENT_ERROR(&info->parser, info->parser.cur_token,
                "Floating point value required");
        }
        return;
    }
    const char *end = qu_parse_float(value, result);
    int vlen = strlen(value);
    if(end != value + vlen) {
        LONGJUMP_WITH_CONTENT_ERROR(&info->parser, info->parser.cur_token,
            "Floating point value required");
    }
}

void qu_eval_str(qu_config_context *info,
    const char *data, int interp, qu_ast_node *node,
    const char **result, int *rlen)
{
    const char *c;
    if(interp && strchr(data, '$')) {
        obstack_blank(info->alloc, 0);
        for(c = data; *c;) {
            if(*c != '$' && *c != '\\') {
                obstack_1grow(info->alloc, *c);
                ++c;
                continue;
            }
            if(*c == '\\') {
                if(!*++c) {
                    LONGJUMP_WITH_CONTENT_ERROR(&info->parser,
                        info->parser.cur_token,
                        "Expected char after backslash");
                }
                obstack_1grow(info->alloc, *c);
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
                    LONGJUMP_WITH_CONTENT_ERROR(&info->parser,
                        info->parser.cur_token,
                        "Expected closing }");
                }
                void *base = obstack_base(&info->parser.pieces);
                variable_t *var = qu_evaluate(info, name, nlen, node);
                qu_append_var_to(var, info->alloc);
                obstack_free(&info->parser.pieces, base);
            } else {
                while(*c && (isalnum(*c) || *c == '_')) ++c;
                nlen = c - name;
                if(nlen == 0) {
                    obstack_1grow(info->alloc, '$');
                    continue;
                }
                const char *value;
                int value_len;
                if(!qu_get_string_len(info, name, nlen, &value, &value_len)) {
                    obstack_grow(info->alloc, value, strlen(value));
                } else {
                    //COYAML_DEBUG("Not found variable ``%.*s''", nlen, name);
                }
            }
        }
        obstack_1grow(info->alloc, 0);
        *rlen = obstack_object_size(info->alloc)-1;
        *result = obstack_finish(info->alloc);
    } else {
        *rlen = strlen(data);
        *result = obstack_copy0(info->alloc, data, *rlen);
    }
}


void qu_node_to_int(qu_config_context *ctx, qu_ast_node *node, long *result) {
    const char *content = qu_node_content(node);
    if(content)
        qu_eval_int(ctx, content, 1, node, result);
}

void qu_node_to_bool(qu_config_context *ctx, qu_ast_node *node, int *result) {
    const char *content = qu_node_content(node);
    if(content)
        qu_eval_bool(ctx, content, 1, node, result);
}

void qu_node_to_float(qu_config_context *ctx, qu_ast_node *node,
    double *result)
{
    const char *content = qu_node_content(node);
    if(content)
        qu_eval_float(ctx, content, 1, node, result);
}

void qu_node_to_str(qu_config_context *ctx, qu_ast_node *node,
    const char **result, int *rlen) {
    const char *content = qu_node_content(node);
    if(content)
        qu_eval_str(ctx, content, 1, node, result, rlen);
}
