#ifndef _H_UTIL
#define _H_UTIL


#define unlikely(a) __builtin_expect((a), 0)
#define likely(a) __builtin_expect((a), 1)


#endif // _H_UTIL
