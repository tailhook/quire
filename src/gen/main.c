#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "../yaml/parser.h"
#include "metadata.h"
#include "header.h"
#include "source.h"
#include "process.h"
#include "../error.h"
#include "../yaml/codes.h"
#include "../raw/common.h"

#define std_assert(val) if((val) == -1) {\
    fprintf(stderr, "quire: %s\n", strerror(errno));\
    exit(1); \
    }



int main(int argc, char **argv) {
    int rc;
    int outputting = 0;
    struct qu_context ctx;
    jmp_buf jmp;
    if(!(rc = setjmp(jmp))) {
        qu_context_init(&ctx, &jmp);
        quire_parse_options(&ctx.options, argc, argv);
        qu_file_parse(&ctx.parser, ctx.options.source_file);
        qu_raw_process(&ctx.parser, ~0);
        qu_config_preprocess(&ctx);

        if(ctx.options.output_header) {
            fflush(stdout);
            int fd = open(ctx.options.output_header, O_WRONLY|O_CREAT, 0666);
            std_assert(fd);
            int rc = dup2(fd, 1);
            std_assert(rc);
            close(fd);
            ctx.out = stdout;  // TODO(tailhook) fix
            ftruncate(1, 0);
            outputting = 1;
            std_assert(qu_output_header(&ctx));
            outputting = 0;
        }

        if(ctx.options.output_source) {
            fflush(stdout);
            int fd = open(ctx.options.output_source, O_WRONLY|O_CREAT, 0666);
            std_assert(fd);
            int rc = dup2(fd, 1);
            std_assert(rc);
            close(fd);
            ctx.out = stdout;  // TODO(tailhook) fix
            ftruncate(1, 0);
            outputting = 1;
            const char *fn = ctx.options.output_header;
            if(!fn) {
                /*  TODO(tailhook) better options? maybe s/.c/.h/ */
                fn = "config.h";
            }
            std_assert(qu_output_source(&ctx, fn));
            outputting = 0;
        }

        if(ctx.errbuf.error)
            longjmp(jmp, 1);
        qu_print_errors(ctx.err, stderr);
        qu_parser_free(&ctx.parser);
    } else {
        if(outputting) {
            fprintf(ctx.out,
                "\n#error Code generation failed. Don't use this file\n");
        }
        if(rc > 0) {
            qu_print_errors(ctx.err, stderr);
            qu_parser_free(&ctx.parser);
            return 1;
        } else if(rc < 0) {
            fprintf(stderr, "quire-gen: Error parsing \"%s\": %s\n",
                ctx.options.source_file, strerror(-rc));
            return 1;
        }
    }
    return 0;
}
