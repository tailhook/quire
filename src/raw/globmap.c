#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stddef.h>

#include "globmap.h"
#include "../yaml/access.h"


qu_ast_node *qu_raw_globmap(qu_parse_context *ctx, qu_ast_node *src) {
    const char *pat = qu_node_content(src);
    const char *pattern = qu_join_filenames(ctx, src->tag->filename, pat);

    const char *star = strchr(pattern, '*');

    if(!star || star != strrchr(pattern, '*')) {
        LONGJUMP_WITH_CONTENT_ERROR(ctx, src->start_token,
            "Exactly one star in glob pattern required");
    }
    const char *startbr = strchr(pattern, '(');
    const char *endbr = strrchr(pattern, ')');
    if(!startbr || startbr != strrchr(pattern, '(') ||
       !endbr || endbr != strchr(pattern, ')')
       || startbr > star || endbr <= star) {
        LONGJUMP_WITH_CONTENT_ERROR(ctx, src->start_token,
            "Exactly one pair of brackets \"(\", \")\" required"
            " and brackets should contain a star pattern");
    }

    // pat_star, pat_end denote the directory entry start and end
    // the bracketed part in example:  some/[pattern*file]/name
    const char *c;
    for (c = star-1; c >= pattern && *c != '/'; --c);
    const char *pat_start = c+1;
    for (c = star+1; *c && *c != '/'; --c);
    const char *pat_end = c;
    int taillen = strlen(pat_end);

    char *dir;
    int dirlen;
    if(pat_start > pattern) {
        // TODO(tailhook) strip off the brackets
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
        LONGJUMP_WITH_SYSTEM_ERROR(ctx, src->start_token,
            "Can't open directory");
    }

    qu_ast_node *result = qu_new_mapping_node(ctx, NULL);

    int name_max = pathconf(dir, _PC_NAME_MAX);
    if (name_max == -1)         /* Limit not defined, or error */
        name_max = 255;         /* Take a guess */
    int len = offsetof(struct dirent, d_name) + name_max + 1;
    struct dirent *entryp = alloca(len);
    while(!readdir_r(d, entryp, &entryp)) {
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
            int suffixlen = pat_end - star+1;
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

        obstack_blank(&ctx->pieces, 0);
        char *nstart = entryp->d_name;
        if(startbr > pat_start) {  // The case "dir/some(thing*)"
            nstart += startbr - pat_start;
        } else if(startbr < pat_start) {  // The case "d(ir/something*)"
            obstack_grow(&ctx->pieces, startbr, pat_start - startbr);
        }
        char *nend = nstart + strlen(entryp->d_name);
        if(endbr < pat_end) {  // The case "(some*thing).conf"
            nend -= pat_end - endbr;
        }
        obstack_grow(&ctx->pieces, nstart, nend - nstart);
        if(endbr > pat_end) { // The case "(some*/thing).conf"
            obstack_grow0(&ctx->pieces, pat_end, endbr - pat_end);
        } else {
            obstack_1grow(&ctx->pieces, 0);
        }


        char *name = obstack_finish(&ctx->pieces);

        qu_ast_node *knode = qu_new_raw_text_node(ctx, name);
        qu_ast_node *next = qu_file_newparse(ctx, fn);
        qu_mapping_add(ctx, result, knode, name, next);
    }
    if(!entryp) {
        LONGJUMP_WITH_SYSTEM_ERROR(ctx, src->start_token,
            "Can't read directory");
    }
    closedir(d);

    return result;
}
