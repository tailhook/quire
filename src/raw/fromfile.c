#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "fromfile.h"
#include "../yaml/parser.h"
#include "../yaml/access.h"

qu_ast_node *qu_raw_fromfile(qu_parse_context *ctx, qu_ast_node *src) {
    int rc, eno;
    unsigned char *data = NULL;

    const char *included_file = qu_node_content(src);
    const char *fn = qu_join_filenames(ctx, src->tag->filename, included_file);

    int fd = open(fn, O_RDONLY);
    if(fd < 0) {
        LONGJUMP_WITH_SYSTEM_ERROR(ctx, src->start_token,
            "Can't open file");
    }
    struct stat stinfo;
    rc = fstat(fd, &stinfo);
    if(rc < 0) {
        eno = errno;
        close(fd);
        LONGJUMP_WITH_SYSTEM_ERROR(ctx, src->start_token,
            "Can't stat file");
    }
    data = obstack_alloc(&ctx->pieces, stinfo.st_size+1);
    int so_far = 0;
    while(so_far < stinfo.st_size) {
        rc = read(fd, data + so_far, stinfo.st_size - so_far);
        if(rc == -1) {
            eno = errno;
            if(eno == EINTR) continue;
            close(fd);
            LONGJUMP_WITH_SYSTEM_ERROR(ctx, src->start_token,
                "Error reading file");
        }
        if(!rc) {
            // WARNING: file truncated
            break;
        }
        so_far += rc;
    }
    close(fd);
    data[so_far] = 0;
    // TODO(tailhook) probably create another node
    src->content = (char *)data;
    src->content_len = so_far;
    src->tag = NULL;
    return src;
}
