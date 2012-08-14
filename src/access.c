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
    // TODO(tailhook) better parse text
    node->content = obstack_copy0(&node->ctx->pieces,
        (char *)node->start_token->data,
        node->start_token->bytelen);
    return node->content;
}

qu_ast_node **qu_find_node(qu_ast_node **root, char *value) {
    while(*root) {
        int cmp = strcmp(qu_node_content(*root), value);
        if(!cmp)
            return root;
        if(cmp < 0)
            root = &(*root)->left;
        if(cmp > 0)
            root = &(*root)->right;
    }
    return root;
}

qu_ast_node *qu_map_get(qu_ast_node *node, char *key) {
    if(node->kind != QU_NODE_MAPPING)
        return NULL;
    qu_ast_node *knode = *qu_find_node(&node->tree, key);
    if(knode)
        return knode->value;
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
       || !strcasecmp(content, "~")) {
       *value = 0;
       return 0;
    }
    if(!strcasecmp(content, "true")
       || !strcasecmp(content, "y")
       || !strcasecmp(content, "on")
       || !strcasecmp(content, "yes")) {
       *value = 1;
       return 0;
    }
    return -1;
}

