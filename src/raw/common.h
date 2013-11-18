#ifndef QUIRE_RAW_COMMON_H
#define QUIRE_RAW_COMMON_H

#include "../yaml/parser.h"

struct qu_var_frame;

const char *qu_join_filenames(struct qu_parser *ctx,
    const char *base, const char *target);

enum {
    QU_RAW_FLAGS_NONE = 0,
    QU_RAW_FLAG_MERGE = 1,
    QU_RAW_FLAG_UNPACK = 2,
    QU_RAW_FLAG_ALIAS = 4,
    QU_RAW_FLAG_INCLUDE = 8,
    QU_RAW_FLAG_VARS = 16,
    QU_RAW_FLAG_NOVARS = 32,
    QU_RAW_FLAG_TEMPLATE = 64
};

void qu_raw_process(struct qu_parser *ctx, struct qu_var_frame *frame,
    unsigned flags);
unsigned qu_raw_flags_from_str(char *flags);

#endif  // QUIRE_RAW_COMMON_H
