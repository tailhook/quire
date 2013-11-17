#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>

#include "include.h"
#include "fromfile.h"
#include "globseq.h"
#include "globmap.h"
#include "merge.h"
#include "../yaml/codes.h"

typedef qu_ast_node *(*qu_directive_processor)(struct qu_parser *ctx,
                                              qu_ast_node *source);

static struct {
    const char *tag;
    unsigned flag;
    qu_directive_processor processor;
} qu_tag_registry[] = {
    {"!Include", QU_RAW_FLAG_INCLUDE, qu_raw_include},
    {"!FromFile", QU_RAW_FLAG_INCLUDE, qu_raw_fromfile},
    {"!GlobSeq", QU_RAW_FLAG_INCLUDE, qu_raw_globseq},
    {"!GlobMap", QU_RAW_FLAG_INCLUDE, qu_raw_globmap},
};

const int qu_tag_registry_len =
    sizeof(qu_tag_registry)/sizeof(qu_tag_registry[0]);



const char *qu_join_filenames(struct qu_parser *ctx,
    const char *base, const char *target)
{
    if(!target)
        return NULL;
    if(*target == '/')
        return target;
    const char *slash = strrchr(base, '/');
    if(!slash)
        return target;
    obstack_blank(&ctx->pieces, 0);
    obstack_grow(&ctx->pieces, base, slash - base + 1);
    obstack_grow0(&ctx->pieces, target, strlen(target));
    return obstack_finish(&ctx->pieces);
}


static qu_ast_node *qu_raw_process_value(struct qu_parser *ctx,
    qu_ast_node *node, unsigned flags)
{
    const char *tag = node->tag;
    int i;
    for (i = 0; i < qu_tag_registry_len; ++i) {
        unsigned flag = qu_tag_registry[i].flag;
        if(flag & flags && !strcmp(qu_tag_registry[i].tag, tag)) {
           return qu_tag_registry[i].processor(ctx, node);
        }
    }
    return node;
}

static void qu_raw_visitor(struct qu_parser *ctx, qu_ast_node *node,
    unsigned flags)
{
    switch(node->kind) {
    case QU_NODE_MAPPING: {
        qu_map_member *item;
        qu_map_index *map = &node->val.map_index;
        for(item = TAILQ_FIRST(&map->items); item;) {
            if(item->value->tag) {
                item->value = qu_raw_process_value(ctx, item->value, flags);
            }
            qu_raw_visitor(ctx, item->value, flags);
            item = TAILQ_NEXT(item, lst);
        }
        } break;
    case QU_NODE_SEQUENCE: {
        qu_seq_index *seq = &node->val.seq_index;
        qu_seq_member *item;
        for(item = TAILQ_FIRST(&seq->items); item;) {
            if(item->value->tag) {
                item->value = qu_raw_process_value(ctx, item->value, flags);
            }
            qu_raw_visitor(ctx, item->value, flags);
            item = TAILQ_NEXT(item, lst);
        }
        } break;
    default:
        break;
    }
}

void qu_raw_process(struct qu_parser *ctx, unsigned flags) {
    qu_raw_visitor (ctx, ctx->document, flags);
    qu_raw_maps_visitor(ctx, ctx->document, flags);
}

unsigned qu_raw_flags_from_str(char *flags) {
    unsigned result = QU_RAW_FLAGS_NONE;
    if(strchr(flags, '*'))
        return ~0;
    if(strchr(flags, 'm'))
        result |= QU_RAW_FLAG_MERGE;
    if(strchr(flags, 'u'))
        result |= QU_RAW_FLAG_UNPACK;
    if(strchr(flags, 'a'))
        result |= QU_RAW_FLAG_ALIAS;
    if(strchr(flags, 'i'))
        result |= QU_RAW_FLAG_INCLUDE;
    if(strchr(flags, 'v'))
        result |= QU_RAW_FLAG_VARS;
    if(*flags == '^')
        return ~result;
    return result;
}
