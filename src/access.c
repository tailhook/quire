#include <stdio.h>
#include <assert.h>

#include "access.h"
#include "yparser.h"
#include "codes.h"


char *qu_node_content(qu_ast_node *node) {
    if(node->kind != QU_NODE_SCALAR)
        return NULL;
    if(node->content)
        return node->content;
    if(node->start_token == node->end_token
        && node->start_token->kind == QU_TOK_PLAINSTRING) {
        node->content = obstack_copy0(&node->ctx->pieces,
            (char *)node->start_token->data,
            node->start_token->bytelen);
        return node->content;
    }
    fprintf(stderr, "Not implemented");
    assert(0);
}

qu_ast_node *qu_map_get(qu_ast_node *node, char *key) {
    if(node->kind != QU_NODE_MAPPING)
        return NULL;
    return NULL;
}

int qu_get_boolean(qu_ast_node *node, int *value) {
    char *content = qu_node_content(node);
    if(!content)
        return -1;
    if(!*content
       || !strcasecmp(content, "false")
       || !strcasecmp(content, "no")
       || !strcasecmp(content, "off")
       || !strcasecmp(content, "n")
       || !strcasecmp(content, "~"))
       return 0;
    if(!strcasecmp(content, "true")
       || !strcasecmp(content, "y")
       || !strcasecmp(content, "on")
       || !strcasecmp(content, "yes"))
       return 1;
    return -1;
}

