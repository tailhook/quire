#include "enum.h"
#include "classes.h"
#include "../struct.h"
#include "../context.h"

struct qu_enum_option {
    char *name;
    int value;
};

struct qu_class_enum {
    int options_len;
    struct qu_enum_option *options;
};

static void qu_enum_init(struct qu_context *, struct qu_class *,
    qu_ast_node *node);

struct qu_class_vptr qu_class_vptr_enum = {
    /*  init  */ qu_enum_init
    };

static void qu_enum_init(struct qu_context *ctx, struct qu_class *cls,
    qu_ast_node *node)
{
    struct qu_class_enum *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_enum));
    cls->classdata = self;
    self->options = NULL;
    self->options_len = 0;
}
