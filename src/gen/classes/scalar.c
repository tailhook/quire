#include "scalar.h"
#include "classes.h"
#include "../struct.h"
#include "../context.h"

struct qu_scalar_tag {
    char *name;
    int value;
};

struct qu_class_scalar {
    int tags_len;
    struct qu_scalar_tag *tags;
};

static void qu_scalar_init(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_scalar = {
    /*  init  */ qu_scalar_init
    };

static void qu_scalar_init(struct qu_context *ctx, struct qu_class *cls) {
    struct qu_class_scalar *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_scalar));
    cls->classdata = self;
    self->tags_len = 0;
    self->tags = NULL;
}
