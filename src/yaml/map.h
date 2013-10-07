#ifndef QUIRE_H_YAML_MAP
#define QUIRE_H_YAML_MAP

#include <sys/queue.h>

struct qu_node_s;

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


#endif // QUIRE_H_YAML_MAP
