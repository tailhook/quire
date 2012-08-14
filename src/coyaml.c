#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "yparser.h"
#include "metadata.h"
#include "preprocessing.h"
#include "genheader.h"

#define std_assert(val) if((val) == -1) {\
    fprintf(stderr, "coyaml: %s", strerror(errno));\
    }


int main(int argc, char **argv) {
    qu_context_t ctx;
    quire_parse_options(&ctx.options, argc, argv);
    qu_init();
    std_assert(qu_context_init(&ctx.parsing));
    std_assert(qu_load_file(&ctx.parsing, ctx.options.source_file));
    std_assert(qu_tokenize(&ctx.parsing));
    if(ctx.parsing.error_kind) {
        fprintf(stderr, "Error parsing file %s:%d: %s\n",
            ctx.parsing.filename, ctx.parsing.error_token->start_line,
            ctx.parsing.error_text);
        exit(1);
    } else {
        std_assert(qu_parse(&ctx.parsing));
        if(ctx.parsing.error_kind) {
            fprintf(stderr, "Error parsing file %s:%d: %s\n",
                ctx.parsing.filename, ctx.parsing.error_token->start_line,
                ctx.parsing.error_text);
            exit(1);
        }
    }
    std_assert(qu_config_preprocess(&ctx));

    if(ctx.options.output_header) {
        int fd = open(ctx.options.output_header, O_WRONLY|O_CREAT, 0666);
        std_assert(fd >= 0)
        int rc = dup2(fd, 1);
        std_assert(rc == 0);
        std_assert(qu_output_header(&ctx));
        ftruncate(1, ftell(stdout));
    }

    std_assert(qu_context_free(&ctx.parsing));
    return 0;
}
