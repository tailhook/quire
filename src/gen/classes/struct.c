#include "struct.h"
#include "classes.h"
#include "../struct.h"
#include "../context.h"
#include "../util/print.h"
#include "../util/print.h"

struct qu_class_struct {
    const char *typename;
    struct qu_config_struct *body;
};

static void qu_struct_init(struct qu_context *, struct qu_class *);

struct qu_class_vptr qu_class_vptr_struct = {
    /*  init  */ qu_struct_init
    };

static void qu_struct_init(struct qu_context *ctx, struct qu_class *cls) {
    struct qu_class_struct *self = obstack_alloc(&ctx->parser.pieces,
        sizeof(struct qu_class_struct));
    cls->classdata = self;
    self->body = NULL;
    self->typename = qu_template_alloc(ctx, "`pref`_`name`",
        "name", cls->name,
        NULL);
    qu_fwdecl_add(ctx, self->typename);
}
