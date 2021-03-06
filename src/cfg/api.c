#include <malloc.h>
#include <setjmp.h>
#include <errno.h>
#include <stdlib.h>

#include "api.h"
#include "context.h"
#include "../quire_int.h"
#include "../raw/common.h"
#include "../yaml/parser.h"

struct qu_config_context *qu_config_parser(
    struct qu_config_head *target, jmp_buf *jmp)
{
    struct qu_config_context *ctx = malloc(sizeof(struct qu_config_context));
    if(!ctx) {
        longjmp(*jmp, -errno);
    }
    qu_config_context_init(ctx, target, jmp);
    return ctx;
}
void qu_config_parser_free(struct qu_config_context *ctx) {
    qu_config_context_free(ctx);
    free(ctx);
}

qu_ast_node *qu_config_parse_yaml(struct qu_config_context *ctx,
    const char *filename)
{
    qu_file_parse(&ctx->parser, filename);
    qu_raw_process(&ctx->parser, ctx->vars, ~0);
    return ctx->parser.document;
}

static void *config_chunk_alloc(qu_config_head *cfg, int size) {
    (void) cfg;
    void *res = malloc(size);
    if(!res) {
        // TODO(tailhook) do a long jump
        fprintf(stderr, "Memory allocation failed without jmp context\n");
        abort();
    }
    return res;
}

static void config_chunk_free(qu_config_head *cfg, void *ptr) {
    (void) cfg;
    free(ptr);
}


void qu_config_init(qu_config_head *cfg, int size) {
    memset(cfg, 0, size);
    obstack_specify_allocation_with_arg(&cfg->pieces, 4096, 0,
        config_chunk_alloc, config_chunk_free, cfg);
}

void qu_config_free(qu_config_head *cfg) {
    obstack_free(&cfg->pieces, NULL);
}


void *qu_config_alloc(struct qu_config_context *ctx, int size) {
    return obstack_alloc(ctx->alloc, size);
}

void qu_node_to_scalar(struct qu_config_context *ctx, qu_ast_node *node,
    const char **target, int *target_len)
{
    *target = obstack_copy0(ctx->alloc, node->content, node->content_len);
    *target_len = node->content_len;
}

void qu_set_string(struct qu_config_context *ctx,
    const char *name, int nlen, const char *data, int dlen) {
    qu_var_set_string(&ctx->parser.pieces, ctx->vars, name, nlen, data, dlen);
}

void qu_report_error(struct qu_config_context *ctx, qu_ast_node *node,
    const char *text) {
    qu_err_node_fatal(ctx->err, node, "%s", text);
}
void qu_cli_error(struct qu_config_context *ctx, const char *opt,
    const char *text) {
    qu_err_cli_fatal(ctx->err, opt, "%s", text);
}

void qu_check_config_errors(struct qu_config_context *ctx) {
    if(ctx->err->error) {
        longjmp(*ctx->err->jmp, 1);
    }
    qu_print_errors(ctx->err, stderr);
}

void qu_print_config_errors(struct qu_config_context *ctx) {
    qu_print_errors(ctx->err, stderr);
}
