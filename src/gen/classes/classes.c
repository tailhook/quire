#include <string.h>

#include "../context.h"
#include "classes.h"
#include "struct.h"
#include "alter.h"
#include "enum.h"
#include "scalar.h"

static struct {
    const char *name;
    struct qu_class_vptr *ptr;
} qu_vpointers[] = {
    {"!Struct", &qu_class_vptr_struct},
    // {"!Alternative", &qu_class_vptr_alternative},
    // {"!Enum", &qu_class_vptr_enum},
    // {"!TagScalar", &qu_class_vptr_scalar}
};
const int qu_vpointers_num = sizeof(qu_vpointers)/sizeof(qu_vpointers[0]);

struct qu_class_vptr *qu_class_get_vptr(const char *name, int namelen)
{
    int i;
    for(i = 0; i < qu_vpointers_num; ++i) {
        if((int)strlen(qu_vpointers[i].name) == namelen &&
           !strncmp(qu_vpointers[i].name, name, namelen))
            return qu_vpointers[i].ptr;
    }
    return NULL;
}

static void qu_classes_decl_visit(struct qu_context *ctx, struct qu_class *cls)
{
    if(!cls)
        return;
    qu_classes_decl_visit(ctx, cls->left);
    cls->vp->func_declare(ctx, cls);
    qu_classes_decl_visit(ctx, cls->right);
}

static void qu_classes_body_visit(struct qu_context *ctx, struct qu_class *cls)
{
    if(!cls)
        return;
    qu_classes_body_visit(ctx, cls->left);
    cls->vp->func_body(ctx, cls);
    qu_classes_body_visit(ctx, cls->right);
}

void qu_classes_print_functions(struct qu_context *ctx) {
    qu_classes_decl_visit(ctx, ctx->class_index.root);
    qu_classes_body_visit(ctx, ctx->class_index.root);
}
