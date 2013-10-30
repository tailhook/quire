#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

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

typedef struct eval_context_s {
    qu_config_context *info;
    const char *data;
    const char *token;
    const char *next;
    const char *end;
    token_t curtok;
} eval_context_t;

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

static struct unit_s {
    const char *unit;
    size_t value;
} units[] = {
    {"k", 1000L},
    {"ki", 1L << 10},
    {"M", 1000000L},
    {"Mi", 1L << 20},
    {"G", 1000000000L},
    {"Gi", 1L << 30},
    {"T", 1000000000000L},
    {"Ti", 1L << 40},
    {"P", 1000000000000000L},
    {"Pi", 1L << 50},
    {"E", 1000000000000000000L},
    {"Ei", 1L << 60},
    {NULL, 0},
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

static void next_tok(eval_context_t *ctx) {
    ctx->token = ctx->next;
    if(ctx->token == ctx->end) {
        ctx->curtok = TOK_END;
    } else {
        ctx->curtok = _next_tok(&ctx->next, ctx->end);
    }
}


static const char *parse_double(const char *value, double *result) {
    const char *end;
    struct unit_s *unit;
    double val = strtod(value, (char **)&end);
    if(*end) {
        for(unit = units; unit->unit; ++unit) {
            if(!strcmp(end, unit->unit)) {
                val *= unit->value;
                end += strlen(unit->unit);
                break;
            }
        }
    }
    *result = val;
    return end;
}

static int var_to_integer(variable_t **var) {
    variable_t *v = *var;
    if(v->type == VAR_INT) return 0;
    if(v->type != VAR_STRING) {
        errno = ENOTSUP;
        return -1;
    }
    char *end;
    long val = strtol(v->data.str.value, &end, 0);
    if(end != v->data.str.value + v->data.str.length) {
        errno = EINVAL;
        return -1;
    }
    v->type = VAR_INT;
    v->data.intvalue = val;
    return 0;
}

static int var_to_string(variable_t **var) {
    if(!*var) return -1;
    if((*var)->type == VAR_STRING) return 0;
    if((*var)->type != VAR_INT) {
        errno = ENOTSUP;
        return -1;
    }
    char buf[128];  // should be enought for long
    int len = sprintf(buf, "%ld", (*var)->data.intvalue);
    if(len <= 0) return -1;
    variable_t *nvar = realloc(*var, sizeof(variable_t)+len+1);
    if(!nvar) return -1;
    char *data = (char*)nvar + sizeof(variable_t);
    memcpy(data, buf, len);
    data[len] = 0;
    nvar->type = VAR_STRING;
    nvar->data.str.value = data;
    nvar->data.str.length = len;
    *var = nvar;
    return 0;
}

static variable_t *eval_sum(eval_context_t *ctx);

static variable_t *eval_atom(eval_context_t *ctx) {
    next_tok(ctx);
    if(ctx->curtok == TOK_LPAREN) {
        variable_t *res = eval_sum(ctx);
        if(!res) return NULL;
        if(ctx->curtok != TOK_RPAREN) {
            free(res);
            errno = EINVAL;
            return NULL;
        }
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_INT) {
        variable_t *res = (variable_t *)malloc(sizeof(variable_t));
        if(!res) return NULL;
        res->type = VAR_INT;
        res->data.intvalue = atoi(ctx->token);
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_STRING) {
        variable_t *res = (variable_t *)malloc(sizeof(variable_t)
            + ctx->next - ctx->token - 1);
        if(!res) return NULL;
        res->type = VAR_STRING;
        char *data = (char *)res + sizeof(variable_t);
        memcpy(data, ctx->token+1, ctx->next - ctx->token - 1);
        data[ctx->next - ctx->token - 2] = 0;
        res->data.str.value = data;
        res->data.str.length = ctx->next - ctx->token;
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_IDENT) {
        const char *value;
        int value_len;
        if(qu_get_string_len(ctx->info, ctx->token, ctx->next - ctx->token,
            &value, &value_len) < 0) {
            return NULL;
        }
        variable_t *res = (variable_t *)malloc(sizeof(variable_t));
        if(!res) return NULL;
        res->type = VAR_STRING;
        res->data.str.value = value;
        res->data.str.length = strlen(value);
        next_tok(ctx);
        return res;
    } else {
        errno = EINVAL;
        return NULL;
    }
}

static variable_t *eval_product(eval_context_t *ctx) {
    variable_t *left = eval_atom(ctx);
    if(!left) return NULL;
    while(ctx->curtok == TOK_PRODUCT
        || ctx->curtok == TOK_DIVISION
        || ctx->curtok == TOK_MODULO) {
        token_t op = ctx->curtok;
        variable_t *right = eval_product(ctx);
        if(!right) {
            free(left);
            return NULL;
        }
        if(var_to_integer(&left) || var_to_integer(&right)) {
            free(left);
            free(right);
            return NULL;
        }
        if(op == TOK_PRODUCT) {
            left->data.intvalue *= right->data.intvalue;
        }
        if(op == TOK_DIVISION) {
            left->data.intvalue /= right->data.intvalue;
        }
        if(op == TOK_MODULO) {
            left->data.intvalue %= right->data.intvalue;
        }
        free(right);
    }
    return left;
}

static variable_t *eval_sum(eval_context_t *ctx) {
    variable_t *left = eval_product(ctx);
    if(!left) return NULL;
    while(ctx->curtok == TOK_PLUS || ctx->curtok == TOK_MINUS) {
        token_t op = ctx->curtok;
        variable_t *right = eval_product(ctx);
        if(!right) {
            free(left);
            return NULL;
        }
        if(var_to_integer(&left) || var_to_integer(&right)) {
            free(left);
            free(right);
            return NULL;
        }
        if(op == TOK_PLUS) {
            left->data.intvalue += right->data.intvalue;
        }
        if(op == TOK_MINUS) {
            left->data.intvalue -= right->data.intvalue;
        }
        free(right);
    }
    return left;
}

static variable_t *evaluate(qu_config_context *info,
    const char *data, int dlen)
{
    eval_context_t eval = {
        .info = info,
        .data = data,
        .end = data+dlen,
        .token = data,
        .next = data,
        .curtok = TOK_NONE
        };
    variable_t *res = eval_sum(&eval);
    if(eval.token != eval.end || eval.curtok != TOK_END) {
        if(res) {
            free(res);
        }
        return NULL;
    }
    return res;
}

void qu_eval_int(qu_config_context *info, const char *value,
    int interp, long *result)
{
    if(interp && strchr(value, '$')) {
        const char *data;
        int dlen;
        qu_eval_str(info, value, interp, &data, &dlen);
        const char *end = qu_parse_int(data, result);
        obstack_free(&info->parser.pieces, (void *)data);
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
    int interp, int *result)
{
    if(interp && strchr(value, '$')) {
        const char *data;
        int dlen;
        qu_eval_str(info, value, interp, &data, &dlen);
        int ok = qu_parse_bool(data, result);
        obstack_free(&info->parser.pieces, (void *)data);
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
    int interp, double *result)
{
    if(interp && strchr(value, '$')) {
        const char *data;
        int dlen;
        qu_eval_str(info, value, interp, &data, &dlen);
        const char *end = parse_double(data, result);
        obstack_free(&info->parser.pieces, (void *)data);
        if(end != data + dlen) {
            LONGJUMP_WITH_CONTENT_ERROR(&info->parser, info->parser.cur_token,
                "Floating point value required");
        }
        return;
    }
    const char *end = parse_double(value, result);
    int vlen = strlen(value);
    if(end != value + vlen) {
		LONGJUMP_WITH_CONTENT_ERROR(&info->parser, info->parser.cur_token,
			"Floating point value required");
	}
}

void qu_eval_str(qu_config_context *info,
    const char *data, int interp, const char **result, int *rlen)
{
    const char *c;
    if(interp && strchr(data, '$')) {
        obstack_blank(&info->parser.pieces, 0);
        for(c = data; *c;) {
            if(*c != '$' && *c != '\\') {
                obstack_1grow(&info->parser.pieces, *c);
                ++c;
                continue;
            }
            if(*c == '\\') {
				if(!*++c) {
					LONGJUMP_WITH_CONTENT_ERROR(&info->parser,
                        info->parser.cur_token,
						"Expected char after backslash");
				}
                obstack_1grow(&info->parser.pieces, *c);
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
                variable_t *var = evaluate(info, name, nlen);
                if(var) {
                    if(var_to_string(&var)) {
                        free(var);
						LONGJUMP_WITH_CONTENT_ERROR(&info->parser,
                            info->parser.cur_token,
							"Variable not found");
                    }
                    obstack_grow(&info->parser.pieces,
                        var->data.str.value, var->data.str.length);
                    free(var);
                }
            } else {
                while(*c && (isalnum(*c) || *c == '_')) ++c;
                nlen = c - name;
                if(nlen == 0) {
                    obstack_1grow(&info->parser.pieces, '$');
                    continue;
                }
                const char *value;
                int value_len;
                if(!qu_get_string_len(info, name, nlen, &value, &value_len)) {
                    obstack_grow(&info->parser.pieces, value, strlen(value));
                } else {
                    //COYAML_DEBUG("Not found variable ``%.*s''", nlen, name);
                }
            }
        }
        obstack_1grow(&info->parser.pieces, 0);
        *rlen = obstack_object_size(&info->parser.pieces)-1;
        *result = obstack_finish(&info->parser.pieces);
    } else {
        *rlen = strlen(data);
        *result = obstack_copy0(&info->parser.pieces, data, *rlen);
    }
}


void qu_node_to_int(qu_config_context *ctx, qu_ast_node *node, long *result) {
    const char *content = qu_node_content(node);
    if(content)
        qu_eval_int(ctx, content, 1, result);
}

void qu_node_to_bool(qu_config_context *ctx, qu_ast_node *node, int *result) {
    const char *content = qu_node_content(node);
    if(content)
        qu_eval_bool(ctx, content, 1, result);
}

void qu_node_to_float(qu_config_context *ctx, qu_ast_node *node,
    double *result)
{
    const char *content = qu_node_content(node);
    if(content)
        qu_eval_float(ctx, content, 1, result);
}

void qu_node_to_str(qu_config_context *ctx, qu_ast_node *node,
    const char **result, int *rlen) {
    const char *content = qu_node_content(node);
    if(content)
        qu_eval_str(ctx, content, 1, result, rlen);
}
