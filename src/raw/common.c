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

typedef qu_ast_node *(*qu_directive_processor)(qu_parse_context *ctx,
                                              qu_ast_node *source);

static struct {
    const char *tag;
    qu_directive_processor processor;
} qu_tag_registry[] = {
    {"!Include", qu_raw_include},
    {"!FromFile", qu_raw_fromfile},
    {"!GlobSeq", qu_raw_globseq},
    {"!GlobMap", qu_raw_globmap},
};



char *qu_join_filenames(qu_parse_context *ctx, char *base, char *target) {
	if(!target)
		return NULL;
	if(*target == '/')
		return target;
	char *slash = strrchr(base, '/');
	if(!slash)
		return target;
	obstack_blank(&ctx->pieces, 0);
	obstack_grow(&ctx->pieces, base, slash - base + 1);
	obstack_grow0(&ctx->pieces, target, strlen(target));
	return obstack_finish(&ctx->pieces);
}


static qu_ast_node *qu_raw_process_value(qu_parse_context *ctx,
    qu_ast_node *node)
{
	int tlen = node->tag->bytelen;
	char *tag = (char *)node->tag->data;
    int i;
    for (i = 0; i < sizeof(qu_tag_registry)/sizeof(qu_tag_registry[0]); ++i) {
        if(tlen == strlen(qu_tag_registry[i].tag)
           && !strncmp(qu_tag_registry[i].tag, tag, tlen)) {
           return qu_tag_registry[i].processor(ctx, node);
        }
    }
	return node;
}

static void qu_raw_visitor(qu_parse_context *ctx, qu_ast_node *node) {
    switch(node->kind) {
    case QU_NODE_MAPPING: {
        qu_map_index *map = &node->val.map_index;
        for(qu_map_member *item = TAILQ_FIRST(&map->items); item;) {
            if(item->value->tag) {
				item->value = qu_raw_process_value(ctx, item->value);
			}
			qu_raw_visitor(ctx, item->value);
			item = TAILQ_NEXT(item, lst);
        }
        } break;
    case QU_NODE_SEQUENCE: {
        qu_seq_index *seq = &node->val.seq_index;
        for(qu_seq_member *item = TAILQ_FIRST(&seq->items); item;) {
            if(item->value->tag) {
				item->value = qu_raw_process_value(ctx, item->value);
			}
			qu_raw_visitor(ctx, item->value);
			item = TAILQ_NEXT(item, lst);
        }
        } break;
    }
}

void qu_raw_process(qu_parse_context *ctx) {
    qu_raw_maps_visitor(ctx, ctx->document);
    qu_raw_merge_maps (ctx, ctx->document);
}
