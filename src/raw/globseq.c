
#include <sys/types.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>

#include "globmap.h"
#include "../error.h"
#include "../yaml/access.h"


qu_ast_node *qu_raw_globseq(struct qu_parser *ctx, qu_ast_node *src) {
    const char *pat = qu_node_content(src);
    const char *pattern = qu_join_filenames(ctx,
        src->tag_token->filename, pat);

    const char *star = strchr(pattern, '*');

    if(!star || star != strrchr(pattern, '*')) {
        qu_err_node_fatal(ctx->err, src,
            "Exactly one star in glob pattern required");
    }

    const char *c;
    for (c = star-1; c >= pattern && *c != '/'; --c);
    const char *pat_start = c+1;
    for (c = star+1; *c && *c != '/'; ++c);
    const char *pat_end = c;
    int taillen = strlen(pat_end);

    char *dir;
    int dirlen;
    if(pat_start > pattern) {
        dirlen = pat_start - pattern;
        dir = alloca(dirlen + 1);
        memcpy(dir, pattern, dirlen);
        dir[dirlen] = 0;
    } else {
        dir = ".";
        dirlen = 1;
    }

    DIR *d = opendir(dir);
    if(!d) {
        qu_err_node_fatal(ctx->err, src,
            "Can't open directory \"%s\"", dir);
    }

    qu_ast_node *result = qu_new_sequence_node(ctx, NULL);

    int name_max = pathconf(dir, _PC_NAME_MAX);
    if (name_max == -1)         /* Limit not defined, or error */
        name_max = 255;         /* Take a guess */
    int len = offsetof(struct dirent, d_name) + name_max + 1;
    struct dirent *entryp = alloca(len);
    int rc;
    while(!(rc = readdir_r(d, entryp, &entryp)) && entryp) {
        // Prefix match
        if(pat_start == star) { //  dir/* shouldn't match hidden files
            if(entryp->d_name[0] == '.')
                continue;
        } else {
            if(strncmp(entryp->d_name, pat_start, star - pat_start))
                continue;
        }

        // Suffix match
        char *eend = entryp->d_name + strlen(entryp->d_name);
        if(pat_end > star+1) {
            int suffixlen = pat_end - (star+1);
            if(eend - entryp->d_name < suffixlen)
                continue;
            if(strncmp(eend - suffixlen, star+1, suffixlen))
                continue;
        }

        obstack_blank(&ctx->pieces, 0);
        obstack_grow(&ctx->pieces, dir, dirlen);
        obstack_grow(&ctx->pieces, entryp->d_name, strlen(entryp->d_name));
        obstack_grow0(&ctx->pieces, pat_end, taillen);
        char *fn = obstack_finish(&ctx->pieces);

        if(access(fn, F_OK) < 0)  // file not exists after adding suffix
            continue;

        qu_ast_node *next = qu_file_newparse(ctx, fn);
        if(next) {
            qu_sequence_add(ctx, result, next);
        }
    }
    if(rc) {
        qu_err_node_fatal(ctx->err, src,
            "Can't read directory \"%s\"", dir);
    }
    closedir(d);

    return result;
}
