#ifndef _H_QUIRE_INT
#define _H_QUIRE_INT

#include <obstack.h>
#include <setjmp.h>

typedef struct __attribute__((__aligned__(512))) qu_config_head {
	jmp_buf safejump;
    struct obstack pieces;
} qu_config_head;

_Static_assert(sizeof(struct qu_config_head) == 512,
    "Wrong size of qu_config_head");

#endif // _H_QUIRE_INT
