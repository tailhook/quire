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
#include "gensource.h"
#include "error.h"
#include "wrappers.h"
#include "codes.h"

#define std_assert(val) if((val) == -1) {\
    fprintf(stderr, "coyaml: %s", strerror(errno));\
    exit(1); \
    }


int main(int argc, char **argv) {
    qu_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    quire_parse_options(&ctx.options, argc, argv);
    int rc = qu_file_parse(&ctx.parsing, ctx.options.source_file);
    if(rc == 0) {
        rc = qu_merge_maps(&ctx.parsing, QU_MFLAG_MAPMERGE
            |QU_MFLAG_SEQMERGE|QU_MFLAG_RESOLVEALIAS);
    }
    if(rc == 0)
        rc = qu_config_preprocess(&ctx);
    if(rc > 0) {
        qu_print_error(&ctx.parsing, stderr);
        qu_context_free(&ctx.parsing);
        return 1;
    } else if(rc < 0) {
        fprintf(stderr, "quire-gen: Error parsing \"%s\": %s\n",
            ctx.options.source_file, strerror(-rc));
        return 1;
    }

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

    qu_context_free(&ctx.parsing);
    return 0;
}
