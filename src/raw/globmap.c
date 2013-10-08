#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stddef.h>
#include <assert.h>

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
    for (c = star+1; *c && *c != '/'; ++c);
    const char *pat_end = c;
    int taillen = strlen(pat_end);

    char *dir;
    int dirlen;
    if(pat_start > pattern) {
        // TODO(tailhook) strip off the brackets
        dirlen = pat_start - pattern;
        dir = alloca(dirlen + 1);
        const char *c;
        char *t = dir;
        for(c = pattern; c < pat_start; ++c)
            if(*c != '(')
                *t++ = *c;
        *t = 0;
        dirlen = t - dir;
    } else {
        dir = ".";
        dirlen = 1;
    }

    char *prefix;
    char prefix_len;
    if(pat_start == star) {
        prefix = "";
        prefix_len = 0;
    } else {
        prefix = alloca(star - pat_start + 1);
        const char *c;
        char *t = prefix;
        for(c = pat_start; c < star; ++c)
            if(*c != '(')
                *t++ = *c;
        *t = 0;
        prefix_len = t - prefix;
    }

    char *suffix;
    char suffix_len;
    if(pat_end == star+1) {
        suffix = "";
        suffix_len = 0;
    } else {
        suffix = alloca(pat_end - (star+1) + 1);
        const char *c;
        char *t = suffix;
        for(c = star+1; c < pat_end; ++c)
            if(*c != ')')
                *t++ = *c;
        *t = 0;
        suffix_len = t - suffix;
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
    int rc;
    while(!(rc = readdir_r(d, entryp, &entryp)) && entryp) {
        // Prefix match
        if(prefix_len == 0) { //  dir/* shouldn't match hidden files
            if(entryp->d_name[0] == '.')
                continue;
        } else {
            if(strncmp(entryp->d_name, prefix, prefix_len))
                continue;
        }

        // Suffix match
        if(suffix_len) {
            char *eend = entryp->d_name + strlen(entryp->d_name);
            if(eend - entryp->d_name < suffix_len)
                continue;
            if(strncmp(eend - suffix_len, suffix, suffix_len))
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
            nend -= pat_end - endbr - 1;
        }
        obstack_grow(&ctx->pieces, nstart, nend - nstart);
        if(endbr > pat_end) { // The case "(some*)/my.conf"
            obstack_grow0(&ctx->pieces, pat_end, endbr - pat_end);
        } else {
            obstack_1grow(&ctx->pieces, 0);
        }
        char *name = obstack_finish(&ctx->pieces);


        qu_ast_node *knode = qu_new_raw_text_node(ctx, name);
        assert(ctx->errjmp);
        qu_ast_node *next = qu_file_newparse(ctx, fn);
        qu_mapping_add(ctx, result, knode, name, next);
    }
    if(rc) {
        LONGJUMP_WITH_SYSTEM_ERROR(ctx, src->start_token,
            "Can't read directory");
    }
    closedir(d);

    return result;
}
