#include "alter.h"
#include "classes.h"
#include "../struct.h"
#include "../context.h"

struct qu_class_alter {
    LIST_HEAD(qu_config_s_opts, qu_config_struct) options;
};

static void qu_alter_init(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_alternative = {
    /*  init  */ qu_alter_init
    };

static void qu_alter_init(struct qu_context *ctx, struct qu_class *cls) {
    struct qu_class_alter *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_alter));
    cls->classdata = self;
    LIST_INIT(&self->options);
}


