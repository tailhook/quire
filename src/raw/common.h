#ifndef QUIRE_COMMON_H
#define QUIRE_COMMON_H

#include "../yaml/parser.h"

/*  Internal utilities  */
const char *qu_join_filenames(qu_parse_context *ctx,
    const char *base, const char *target);

/*   Public  API  */
void qu_raw_process(qu_parse_context *ctx);

#endif  // QUIRE_COMMON_H
