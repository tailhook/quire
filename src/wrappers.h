#ifndef _H_WRAPPERS
#define _H_WRAPPERS

#include "context.h"

// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
int qu_merge_maps(qu_parse_context *ctx, int flags);
int qu_process_includes(qu_parse_context *ctx, int flags);

#endif //_H_WRAPPERS
