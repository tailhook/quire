#include <string.h>

#include "special.h"
#include "../classes/classes.h"

void qu_special_parse(struct qu_context *ctx,
    const char *name, qu_ast_node *node)
{
    if(!strcmp(name, "__types__")) {
        qu_special_types(ctx, node);
    }

    /*  Unknown keys are just ignored  */
}
