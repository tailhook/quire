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
    obstack_blank(&node->ctx->pieces, 0);
    for(char *c = (char *)node->start_token->data,
             *end = c+node->start_token->bytelen;
        c < end; ++c) {
        if(*c == '"')
            continue;
        else if(*c == '\\' && *(c+1))
            switch(*++c) {
            case 'n': obstack_1grow(&node->ctx->pieces, '\n'); break;
            case 'r': obstack_1grow(&node->ctx->pieces, '\r'); break;
            default: obstack_1grow(&node->ctx->pieces, *c); break;
            }
        else
            obstack_1grow(&node->ctx->pieces, *c);
    }
    obstack_1grow(&node->ctx->pieces, 0);

    node->content = obstack_finish(&node->ctx->pieces);
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

qu_ast_node *qu_get_root(qu_parse_context *ctx) {
    return ctx->document;
}
