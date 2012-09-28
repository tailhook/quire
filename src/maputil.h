#ifndef _H_MAPUTIL
#define _H_MAPUTIL

#include <sys/queue.h>
#include "yparser.h"

typedef struct qu_map_member {
    struct qu_map_member *left;
    struct qu_map_member *right;
    struct qu_node_s *key;
    struct qu_node_s *value;
    TAILQ_ENTRY(qu_map_member) lst;
} qu_map_member;

typedef struct qu_map_index {
    struct qu_map_member *tree;
    TAILQ_HEAD(qu_map_list, qu_map_member) items;
} qu_map_index;

typedef struct qu_seq_member {
    struct qu_node_s *value;
    TAILQ_ENTRY(qu_seq_member) lst;
} qu_seq_member;

typedef struct qu_seq_index {
    TAILQ_HEAD(qu_seq_list, qu_seq_member) items;
} qu_seq_index;

void _qu_merge_maps(struct qu_parse_context_s *ctx, int flags);

#endif  // _H_MAPUTIL
