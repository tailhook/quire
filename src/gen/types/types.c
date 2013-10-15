#include "types.h"
#include "../context.h"

#include "int.h"

static struct {
    const char *tag;
    struct qu_option_vptr *vp;
} qu_types_table[] = {
    {"!Int", &qu_int_vptr}
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

struct qu_option *qu_option_resolve(struct qu_context *ctx,
    const char *tag, int taglen)
{
    int i;
    struct qu_option *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_option));
    self->description = NULL;
    self->typedata = NULL;
    self->example = NULL;
    self->has_default = 0;

    for(i = 0; i < qu_types_num; ++i) {
        if(strlen(qu_types_table[i].tag) == taglen
           && !strncmp(qu_types_table[i].tag, tag, taglen)) {
            self->vp = qu_types_table[i].vp;
        }
    }

    return self;
}
