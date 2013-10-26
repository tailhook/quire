#ifndef QUIRE_H_QUIRE_INT
#define QUIRE_H_QUIRE_INT

#include <obstack.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include "yaml/codes.h"
#include "gen/util/print.h"
#include "error.h"

typedef struct __attribute__((__aligned__(512))) qu_config_head {
    struct obstack pieces;
} qu_config_head;

_Static_assert(sizeof(struct qu_config_head) == 512,
    "Wrong size of qu_config_head");

#define LONGJUMP_WITH_ERRNO(ctx) LONGJUMP_WITH_ERRCODE((ctx), (errno))
#define QU_LONGJUMP(ctx) \
    if((ctx)->errjmp) { \
        longjmp(*(ctx)->errjmp, QU_YAML_ERROR); \
    } else { \
        qu_print_error((ctx), stderr); \
        abort(); \
    }
#define LONGJUMP_WITH_ERRCODE(ctx, code) do {\
    (ctx)->error_kind = QU_ERR_ERRNO; \
    (ctx)->error_text = strerror(code); \
    (ctx)->err_ptr.errnum = code; \
    QU_LONGJUMP((ctx)); \
    } while(0);
#define LONGJUMP_WITH_SCANNER_ERROR(ctx, tok, text) do {\
    (ctx)->error_kind = QU_ERR_SYNTAX; \
    (ctx)->error_text = (text); \
    (ctx)->err_ptr.token = (tok); \
    QU_LONGJUMP((ctx)); \
    } while(0);
#define LONGJUMP_WITH_CONTENT_ERROR(ctx, tok, text) do {\
    (ctx)->error_kind = QU_ERR_VALUE; \
    (ctx)->error_text = (text); \
    (ctx)->err_ptr.token = (tok); \
    QU_LONGJUMP((ctx)); \
    } while(0);
#define LONGJUMP_WITH_PARSER_ERROR(ctx, tok, text) do {\
    (ctx)->error_kind = QU_ERR_SYNTAX; \
    (ctx)->error_text = (text); \
    (ctx)->err_ptr.token = (tok); \
    QU_LONGJUMP((ctx)); \
    } while(0);
#define LONGJUMP_WITH_SYSTEM_ERROR(ctx, tok, text) do {\
    (ctx)->error_kind = QU_ERR_SYSTEM; \
    (ctx)->error_text = (text); \
    (ctx)->err_ptr.token = (tok); \
    QU_LONGJUMP((ctx)); \
    } while(0);

#define LONGJUMP_ERR_NODE(ctx, node, text, ...) do{\
    const char *errtext = qu_template_alloc(ctx, text, ## __VA_ARGS__, NULL); \
    (ctx)->parser.error_kind = YAML_CONTENT_ERROR; \
    (ctx)->parser.error_text = errtext; \
    (ctx)->parser.err_ptr.token = (node)->tag_token ? \
        (node)->tag_token : (node)->start_token; \
    QU_LONGJUMP(&(ctx)->parser); \
    } while(0);

#endif  /*  QUIRE_H_QUIRE_INT */
