#include <assert.h>
#include "types.h"
#include "../context.h"

#include "int.h"
#include "str.h"
#include "bool.h"
#include "float.h"
#include "array.h"
#include "map.h"
#include "type.h"

static struct {
    const char *tag;
    struct qu_option_vptr *vp;
} qu_types_table[] = {
    {"!Int", &qu_int_vptr},
    {"!String", &qu_str_vptr},
    {"!Bool", &qu_bool_vptr},
    {"!Float", &qu_float_vptr},
    {"!Array", &qu_array_vptr},
    {"!Mapping", &qu_map_vptr},
    {"!Type", &qu_type_vptr}
};
const int qu_types_num = sizeof(qu_types_table)/sizeof(qu_types_table[0]);

struct qu_option *qu_option_new(struct qu_context *ctx,
    struct qu_option_vptr *vp)
{
    struct qu_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_option));
    self->vp = vp;
    return self;
}

struct qu_option *qu_option_resolve(struct qu_context *ctx, const char *tag)
{
    int i;
    struct qu_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_option));
    self->description = NULL;
    self->typedata = NULL;
    self->example = NULL;
    self->has_default = 0;
    self->vp = NULL;
    self->cli_only = 0;

    for(i = 0; i < qu_types_num; ++i) {
        if(!strcmp(qu_types_table[i].tag, tag)) {
            self->vp = qu_types_table[i].vp;
            break;
        }
    }

    return self;
}
