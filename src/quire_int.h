#ifndef QUIRE_H_QUIRE_INT
#define QUIRE_H_QUIRE_INT

#include <obstack.h>

typedef struct __attribute__((__aligned__(512))) qu_config_head {
    struct obstack pieces;
} qu_config_head;

_Static_assert(sizeof(struct qu_config_head) == 512,
    "Wrong size of qu_config_head");

#endif  /*  QUIRE_H_QUIRE_INT */
