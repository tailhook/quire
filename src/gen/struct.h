#ifndef QUIRE_H_GEN_STRUCT
#define QUIRE_H_GEN_STRUCT

#include <sys/queue.h>

struct qu_context;

struct qu_config_struct {
    LIST_HEAD(qu_config_s_list, qu_config_struct) children;
    LIST_ENTRY(qu_config_ss_list) lst;
    struct qu_config_struct *parent;
};

struct qu_config_struct *qu_struct_new_root(struct qu_context *ctx);


#endif  // QUIRE_H_GEN_STRUCT
