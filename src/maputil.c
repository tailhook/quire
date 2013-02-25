#include <stdio.h>

#include "context.h"
#include "yparser.h"
#include "maputil.h"
#include "codes.h"
#include "access.h"

static void merge_mapping(qu_parse_context *ctx, qu_map_index *idx,
    qu_ast_node *source, qu_map_member *after) {
    qu_map_member *item;
    TAILQ_FOREACH(item, &source->val.map_index.items, lst) {
        char *key = qu_node_content(item->key);
        qu_map_member **tmp = qu_find_node(&idx->tree, key);
        if(*tmp) // already has a key
            continue;
        qu_map_member *copy = obstack_alloc(&ctx->pieces,
                                            sizeof(qu_map_member));
        copy->key = item->key;
        copy->value = item->value;
        *tmp = copy;
        TAILQ_INSERT_AFTER(&idx->items, after, copy, lst);
        after = copy;
    }
}

static void visitor(qu_parse_context *ctx, qu_ast_node *node, int flags) {
    switch(node->kind) {
    case QU_NODE_MAPPING: {
        qu_map_index *map = &node->val.map_index;
        for(qu_map_member *item = TAILQ_FIRST(&map->items); item;) {
            if(item->value->kind == QU_NODE_ALIAS
               && QU_MFLAG_RESOLVEALIAS & flags) {
                item->value = item->value->val.alias_target;
            }
            if(QU_MFLAG_MAPMERGE & flags
               && !strcmp(qu_node_content(item->key), "<<")) {
                if(item->value->kind == QU_NODE_SEQUENCE) {
                    qu_seq_index *chseq = &item->value->val.seq_index;
                    qu_seq_member *child;
                    TAILQ_FOREACH(child, &chseq->items, lst) {
                        if(child->value->kind == QU_NODE_MAPPING) {
                            merge_mapping(ctx, map, child->value, item);
                        }
                    }
                } else if(item->value->kind == QU_NODE_MAPPING) {
                    merge_mapping(ctx, map, item->value, item);
                }
                qu_map_member *oitem = item;
                item = TAILQ_NEXT(item, lst);
                TAILQ_REMOVE(&map->items, oitem, lst);
            } else {
                visitor(ctx, item->value, flags);
                item = TAILQ_NEXT(item, lst);
            }
        }
        } break;
    case QU_NODE_SEQUENCE: {
        qu_seq_index *seq = &node->val.seq_index;
        for(qu_seq_member *item = TAILQ_FIRST(&seq->items); item;) {
            if(item->value->kind == QU_NODE_ALIAS
               && QU_MFLAG_RESOLVEALIAS & flags) {
                item->value = item->value->val.alias_target;
            }
            if(QU_MFLAG_SEQMERGE & flags
                && item->value->tag
                && item->value->tag->bytelen == 2
                && !memcmp(item->value->tag->data, "!*", 2)) {
                if(item->value->kind == QU_NODE_SEQUENCE) {
                    qu_seq_index *chseq = &item->value->val.seq_index;
                    qu_seq_member *child;
                    TAILQ_FOREACH(child, &chseq->items, lst) {
                        qu_seq_member *copy = obstack_alloc(&ctx->pieces,
                            sizeof(qu_seq_member));
                        copy->value = item->value;
                        TAILQ_INSERT_AFTER(&seq->items, item, copy, lst);
                    }
                }
                qu_seq_member *oitem = item;
                item = TAILQ_NEXT(item, lst);
                TAILQ_REMOVE(&seq->items, oitem, lst);
            } else {
                visitor(ctx, item->value, flags);
                item = TAILQ_NEXT(item, lst);
            }
            // TODO(tailhook) write some tests
        }
        } break;
    }
}

void _qu_merge_maps(struct qu_parse_context_s *ctx, int flags) {
    visitor(ctx, ctx->document, flags);
}

