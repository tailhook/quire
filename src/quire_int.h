#ifndef _H_QUIRE_INT
#define _H_QUIRE_INT

#include <obstack.h>

typedef struct __attribute__((__aligned__(128))) qu_config_head {
    struct obstack pieces;
} qu_config_head;

_Static_assert(sizeof(struct qu_config_head) == 128,
    "Wrong size of qu_config_head");

#endif // _H_QUIRE_INT
