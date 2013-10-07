#ifndef QUIRE_H_YAML_SEQ
#define QUIRE_H_YAML_SEQ

#include <sys/queue.h>

struct qu_node_s;

typedef struct qu_seq_member {
    struct qu_node_s *value;
    TAILQ_ENTRY(qu_seq_member) lst;
} qu_seq_member;

typedef struct qu_seq_index {
    TAILQ_HEAD(qu_seq_list, qu_seq_member) items;
} qu_seq_index;


#endif // QUIRE_H_YAML_SEQ

