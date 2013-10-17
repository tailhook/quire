#ifndef _H_QUIRE_INT
#define _H_QUIRE_INT

#include <obstack.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include "yaml/codes.h"
#include "gen/util/print.h"

typedef struct __attribute__((__aligned__(512))) qu_config_head {
    jmp_buf *errjmp;
    jmp_buf errjmp_buf;
    struct obstack pieces;
} qu_config_head;

_Static_assert(sizeof(struct qu_config_head) == 512,
    "Wrong size of qu_config_head");

#define LONGJUMP_WITH_ERRNO(ctx) LONGJUMP_WITH_ERRCODE((ctx), (errno))
#define LONGJUMP_WITH_ERRCODE(ctx, code) {\
    if((ctx)->errjmp) { \
        longjmp(*(ctx)->errjmp, (code)); \
    } else { \
        fprintf(stderr, "Runtime error: %s\n", strerror(code)); \
        abort(); \
    }}
#define LONGJUMP_WITH_SCANNER_ERROR(ctx, token, text) {\
    if((ctx)->errjmp) { \
        (ctx)->error_kind = YAML_SCANNER_ERROR; \
        (ctx)->error_text = (text); \
        (ctx)->error_token = (token); \
        longjmp(*(ctx)->errjmp, QU_YAML_ERROR); \
    } else { \
        fprintf(stderr, "Scanner error: %s\n", (text)); \
        abort(); \
    }}
#define LONGJUMP_WITH_CONTENT_ERROR(ctx, token, text) {\
    if((ctx)->errjmp) { \
        (ctx)->error_kind = YAML_CONTENT_ERROR; \
        (ctx)->error_text = (text); \
        (ctx)->error_token = (token); \
        longjmp(*(ctx)->errjmp, QU_YAML_ERROR); \
    } else { \
        fprintf(stderr, "Parser error: %s\n", (text)); \
        abort(); \
    }}
#define LONGJUMP_WITH_PARSER_ERROR(ctx, token, text) {\
    if((ctx)->errjmp) { \
        (ctx)->error_kind = YAML_PARSER_ERROR; \
        (ctx)->error_text = (text); \
        (ctx)->error_token = (token); \
        longjmp(*(ctx)->errjmp, QU_YAML_ERROR); \
    } else { \
        fprintf(stderr, "Parser error: %s\n", (text)); \
        abort(); \
    }}

#define LONGJUMP_WITH_SYSTEM_ERROR(ctx, token, text) {\
    if((ctx)->errjmp) { \
        (ctx)->error_kind = YAML_SYSTEM_ERROR; \
        (ctx)->error_text = (text); \
        (ctx)->error_token = (token); \
        longjmp(*(ctx)->errjmp, QU_YAML_ERROR); \
    } else { \
        fprintf(stderr, "System error: %s\n", (text)); \
        abort(); \
    }}

#define LONGJUMP_ERR_NODE(ctx, node, text, ...) {\
    const char *errtext = qu_template_alloc(ctx, text, ## __VA_ARGS__, NULL); \
    if((ctx)->parser.errjmp) { \
        (ctx)->parser.error_kind = YAML_CONTENT_ERROR; \
        (ctx)->parser.error_text = errtext; \
        (ctx)->parser.error_token = (node)->tag_token ? \
            (node)->tag_token : (node)->start_token; \
        longjmp(*(ctx)->parser.errjmp, QU_YAML_ERROR); \
    } else { \
        fprintf(stderr, "Parser error: %s\n", errtext); \
        abort(); \
    }}

#endif // _H_QUIRE_INT
