#include <string.h>

#include "special.h"
#include "types.h"
#include "include.h"
#include "../classes/classes.h"
#include "../../yaml/access.h"

void qu_special_parse(struct qu_context *ctx,
    const char *name, qu_ast_node *node)
{
    if(!strcmp(name, "__types__")) {
        qu_special_types(ctx, node);
    } else if(!strcmp(name, "__include__")) {
        qu_special_include(ctx, node);
    } else if(node->kind == QU_NODE_MAPPING) {
        /*  Unknown keys are just ignored  */
        /*  But unknown mappings should be visited in depth */
        qu_map_member *item;
        TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
            const char *mname = qu_node_content(item->key);
            if(!*mname || *mname == '_') {
                qu_special_parse(ctx, mname, item->value);
            }
        }
    }

}
