#include "../yaml/parser.h"
#include "../yaml/map.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"

static void qu_raw_merge_mapping(qu_parse_context *ctx, qu_map_index *idx,
    qu_ast_node *source, qu_map_member *after) {
    qu_map_member *item;
    TAILQ_FOREACH(item, &source->val.map_index.items, lst) {
        const char *key = qu_node_content(item->key);
        qu_map_member **tmp = NULL;
        if(strcmp(key, "<<")) {
            // we already know that merge key is in the mapping
            // we need to insert it to recurse into more mappings
            // however no need to insert it into tree
            tmp = qu_find_node(&idx->tree, key);
            if(*tmp) // already has a key
                continue;
        }
        qu_map_member *copy = obstack_alloc(&ctx->pieces,
                                            sizeof(qu_map_member));
        copy->key = item->key;
        copy->value = item->value;
        copy->left = NULL;
        copy->right = NULL;
        if(tmp) {
            *tmp = copy;
        }
        TAILQ_INSERT_AFTER(&idx->items, after, copy, lst);
        after = copy;
    }
}

static void qu_raw_merge_sequence(qu_parse_context *ctx, qu_seq_index *idx,
    qu_ast_node *source, qu_seq_member *after) {
    qu_seq_member *item;
    TAILQ_FOREACH(item, &source->val.seq_index.items, lst) {
        qu_seq_member *copy = obstack_alloc(&ctx->pieces,
            sizeof(qu_seq_member));
        copy->value = item->value;
        TAILQ_INSERT_AFTER(&idx->items, after, copy, lst);
        after = copy;
    }
}


void qu_raw_maps_visitor(qu_parse_context *ctx, qu_ast_node *node)
{
    switch(node->kind) {
    case QU_NODE_MAPPING: {
        qu_map_index *map = &node->val.map_index;
        qu_map_member *item;
        for(item = TAILQ_FIRST(&map->items); item;) {
            if(item->value->kind == QU_NODE_ALIAS) {
                item->value = item->value->val.alias_target;
            }
            if(!strcmp(qu_node_content(item->key), "<<")) {
                if(item->value->kind == QU_NODE_SEQUENCE) {
                    qu_map_member *anchor = item;
                    qu_map_member *next = TAILQ_NEXT(item, lst);
                    qu_seq_index *chseq = &item->value->val.seq_index;
                    qu_seq_member *child;
                    TAILQ_FOREACH(child, &chseq->items, lst) {
                        if(child->value->kind == QU_NODE_ALIAS) {
                            child->value = child->value->val.alias_target;
                        }
                        if(child->value->kind == QU_NODE_MAPPING) {
                            qu_raw_merge_mapping(ctx,
                                map, child->value, anchor);
                        }
                        if(next) {
                            anchor = TAILQ_PREV(next, qu_map_list, lst);
                        } else {
                            anchor = TAILQ_LAST(&map->items, qu_map_list);
                        }
                    }
                } else if(item->value->kind == QU_NODE_MAPPING) {
                    qu_raw_merge_mapping(ctx, map, item->value, item);
                }
                qu_map_member *oitem = item;
                item = TAILQ_NEXT(item, lst);
                TAILQ_REMOVE(&map->items, oitem, lst);
            } else {
                qu_raw_maps_visitor(ctx, item->value);
                item = TAILQ_NEXT(item, lst);
            }
        }
        } break;
    case QU_NODE_SEQUENCE: {
        qu_seq_index *seq = &node->val.seq_index;
        qu_seq_member *item;
        for(item = TAILQ_FIRST(&seq->items); item;) {
            if(item->value->kind == QU_NODE_ALIAS) {
                item->value = item->value->val.alias_target;
            }
            if(item->value->tag && !strcmp(item->value->tag, "!Unpack")) {
                if(item->value->kind == QU_NODE_SEQUENCE) {
                    qu_seq_member *anchor = item;
                    qu_seq_member *next = TAILQ_NEXT(item, lst);
                    qu_seq_index *chseq = &item->value->val.seq_index;
                    qu_seq_member *child;
                    TAILQ_FOREACH(child, &chseq->items, lst) {
                        if(child->value->kind == QU_NODE_ALIAS) {
                            child->value = child->value->val.alias_target;
                        }
                        if(child->value->kind == QU_NODE_SEQUENCE) {
                            qu_raw_merge_sequence(ctx,
                                seq, child->value, anchor);
                        }
                        if(next) {
                            anchor = TAILQ_PREV(next, qu_seq_list, lst);
                        } else {
                            anchor = TAILQ_LAST(&seq->items, qu_seq_list);
                        }
                    }
                } // TODO(tailhook) report error
                qu_seq_member *oitem = item;
                item = TAILQ_NEXT(item, lst);
                TAILQ_REMOVE(&seq->items, oitem, lst);
            } else {
                qu_raw_maps_visitor(ctx, item->value);
                item = TAILQ_NEXT(item, lst);
            }
        }
        } break;
    default:
        break;
    }
}


