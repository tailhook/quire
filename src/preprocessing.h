#ifndef _H_PREPROCESSING
#define _H_PREPROCESSING

#include "context.h"

#define QU_TYP_UNKNOWN  0
#define QU_TYP_INT      1
#define QU_TYP_FLOAT    2
#define QU_TYP_FILE     3
#define QU_TYP_DIR      4
#define QU_TYP_STRING   5
#define QU_TYP_BOOL     6
#define QU_TYP_STRUCT   7


int qu_config_preprocess(qu_context_t *ctx);

#endif // _H_PREPROCESSING
