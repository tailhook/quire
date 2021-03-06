#include <string.h>

#include "../context.h"
#include "../util/print.h"
#include "../../yaml/access.h"
#include "classes.h"
#include "struct.h"
#include "choice.h"
#include "enum.h"
#include "scalar.h"
#include "field.h"

static struct {
    const char *name;
    struct qu_class_vptr *ptr;
} qu_vpointers[] = {
    {"!Struct", &qu_class_vptr_struct},
    {"!Choice", &qu_class_vptr_choice},
    {"!Enum", &qu_class_vptr_enum},
    {"!TagScalar", &qu_class_vptr_scalar},
    {"!Field", &qu_class_vptr_field}
};
const int qu_vpointers_num = sizeof(qu_vpointers)/sizeof(qu_vpointers[0]);

struct qu_class_vptr *qu_class_get_vptr(const char *name)
{
    int i;
    for(i = 0; i < qu_vpointers_num; ++i) {
        if(!strcmp(qu_vpointers[i].name, name))
            return qu_vpointers[i].ptr;
    }
    return NULL;
}

static void qu_classes_decl_visit(struct qu_context *ctx, struct qu_class *cls)
{
    if(!cls)
        return;
    qu_classes_decl_visit(ctx, cls->left);
    cls->vp->func_decl(ctx, cls);
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

void qu_classes_print_cdecls(struct qu_context *ctx, qu_ast_node *source) {
    if(source->kind != QU_NODE_MAPPING)
        return;
    qu_map_member *item;
    TAILQ_FOREACH(item, &source->val.map_index.items, lst) {
        const char *mname = qu_node_content(item->key);
        if(mname[0] == '_' && mname[1] != '_' && item->value->tag &&
            !strcmp(item->value->tag, "!CDecl")) {
            qu_code_print(ctx, "${typ} ${varname:c};\n"
                , "typ", qu_node_content(item->value)
                , "varname", mname
                , NULL);
        }
    }
}
