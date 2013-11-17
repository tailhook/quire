#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "../error.h"
#include "fromfile.h"
#include "../yaml/parser.h"
#include "../yaml/access.h"

qu_ast_node *qu_raw_fromfile(struct qu_parser *ctx, qu_ast_node *src) {
    int rc, eno;
    unsigned char *data = NULL;

    const char *included_file = qu_node_content(src);
    const char *fn = qu_join_filenames(ctx,
        src->tag_token->filename, included_file);

    int fd = open(fn, O_RDONLY);
    if(fd < 0) {
        qu_err_system_error(ctx->err, errno, "Can't open file \"%s\"", fn);
        return src;
    }
    struct stat stinfo;
    rc = fstat(fd, &stinfo);
    if(rc < 0) {
        eno = errno;
        close(fd);
        qu_err_system_error(ctx->err, eno, "Can't stat file \"%s\"", fn);
        return src;
    }
    data = obstack_alloc(&ctx->pieces, stinfo.st_size+1);
    int so_far = 0;
    while(so_far < stinfo.st_size) {
        rc = read(fd, data + so_far, stinfo.st_size - so_far);
        if(rc == -1) {
            eno = errno;
            if(eno == EINTR) continue;
            close(fd);
            qu_err_system_error(ctx->err, eno,
                "Error reading file \"%s\"", fn);
            return src;
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
