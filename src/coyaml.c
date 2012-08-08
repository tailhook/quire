#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

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
    } else {
        std_assert(qu_parse(&ctx.parsing));
    }
    std_assert(qu_config_preprocess(&ctx));

    if(ctx.options.output_header) {
        std_assert(qu_output_header(&ctx));
    }

    std_assert(qu_context_free(&ctx.parsing));
    return 0;
}
