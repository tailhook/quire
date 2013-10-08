#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "../yaml/parser.h"
#include "metadata.h"
#include "preprocessing.h"
#include "header.h"
#include "source.h"
#include "../error.h"
#include "../yaml/codes.h"
#include "../raw/common.h"

#define std_assert(val) if((val) == -1) {\
    fprintf(stderr, "coyaml: %s", strerror(errno));\
    exit(1); \
    }

void qu_context_init(qu_context_t *ctx, jmp_buf *jmp) {
    memset(ctx, 0, sizeof(qu_context_t));
    TAILQ_INIT(&ctx->arrays);
    TAILQ_INIT(&ctx->mappings);
    qu_parser_init(&ctx->parsing, jmp);
}


int main(int argc, char **argv) {
    int rc;
    struct qu_context ctx;
    jmp_buf jmp;
    if(!(rc = setjmp(jmp))) {
        qu_context_init(&ctx, &jmp);
        quire_parse_options(&ctx.options, argc, argv);
        qu_file_parse(&ctx.parsing, ctx.options.source_file);
        qu_raw_process(&ctx.parsing);
        qu_config_preprocess(&ctx);

        if(ctx.options.output_header) {
            fflush(stdout);
            int fd = open(ctx.options.output_header, O_WRONLY|O_CREAT, 0666);
            std_assert(fd >= 0)
            int rc = dup2(fd, 1);
            std_assert(rc == 0);
            close(fd);
            std_assert(qu_output_header(&ctx));
            ftruncate(1, ftell(stdout));
        }

        if(ctx.options.output_source) {
            fflush(stdout);
            int fd = open(ctx.options.output_source, O_WRONLY|O_CREAT, 0666);
            std_assert(fd >= 0)
            int rc = dup2(fd, 1);
            std_assert(rc == 0);
            close(fd);
            std_assert(qu_output_source(&ctx));
            ftruncate(1, ftell(stdout));
        }

        qu_parser_free(&ctx.parsing);
        return 0;
    } else {
        if(rc > 0) {
            qu_print_error(&ctx.parsing, stderr);
            qu_parser_free(&ctx.parsing);
            return 1;
        } else if(rc < 0) {
            fprintf(stderr, "quire-gen: Error parsing \"%s\": %s\n",
                ctx.options.source_file, strerror(-rc));
            return 1;
        }
    }
}
