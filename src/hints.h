#ifndef QUIRE_H_HINTS
#define QUIRE_H_HINTS

#define QU_A_FORMAT(idx) __attribute__((format(printf,idx,idx+1)))
#define QU_A_USE_RESULT __attribute__((warn_unused_result))
#define QU_A_NO_RETURN __attribute__((noreturn))

#endif  /* QUIRE_H_HINTS */
