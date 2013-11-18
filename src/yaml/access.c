#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

#include "access.h"
#include "codes.h"


const char *qu_node_content(qu_ast_node *node) {
    if(node->kind == QU_NODE_ALIAS)
        return qu_node_content(node->val.alias_target);
    if(node->kind != QU_NODE_SCALAR) {
        if(node->kind == QU_NODE_MAPPING) {
            qu_ast_node *value = qu_map_get(node, "=");
            if(value)
                return qu_node_content(value);
        }
        return NULL;
    }
    if(!node->content)
        node->content = qu_parse_content(node, &node->ctx->pieces);
    return node->content;
}

const char *qu_parse_content(qu_ast_node *node, struct obstack *buf) {
    char *c, *end;
    if(!node->start_token)
        return "";
    if(node->start_token == node->end_token
        && node->start_token->kind == QU_TOK_PLAINSTRING) {
        node->content = obstack_copy0(buf,
            (char *)node->start_token->data,
            node->start_token->bytelen);
        return node->content;
    }
    // TODO(tailhook) better parse text
    obstack_blank(buf, 0);
    for(c = (char *)node->start_token->data,
             end = c+node->start_token->bytelen;
        c < end; ++c) {
        if(*c == '"')
            continue;
        else if(*c == '\\' && *(c+1))
            switch(*++c) {
            case 'n': obstack_1grow(buf, '\n'); break;
            case 'r': obstack_1grow(buf, '\r'); break;
            default: obstack_1grow(buf, *c); break;
            }
        else
            obstack_1grow(buf, *c);
    }
    obstack_1grow(buf, 0);
    return obstack_finish(buf);
}

qu_map_member **qu_find_node(qu_map_member **root, const char *value) {
    while(*root) {
        int cmp = strcmp(qu_node_content((*root)->key), value);
        if(!cmp)
            return root;
        if(cmp < 0)
            root = &(*root)->left;
        if(cmp > 0)
            root = &(*root)->right;
    }
    return root;
}

qu_ast_node *qu_map_get(qu_ast_node *node, const char *key) {
    if(node->kind == QU_NODE_ALIAS)
        return qu_map_get(node->val.alias_target, key);
    if(node->kind != QU_NODE_MAPPING)
        return NULL;
    qu_map_member *knode = *qu_find_node(&node->val.map_index.tree, key);
    if(knode)
        return knode->value;
    return NULL;
}

qu_ast_node *qu_map_get_len(qu_ast_node *node, const char *key, int klen) {
    if(node->kind == QU_NODE_ALIAS)
        return qu_map_get_len(node->val.alias_target, key, klen);
    if(node->kind != QU_NODE_MAPPING)
        return NULL;

    /* TODO(tailhoook) implement search for key with length */
    char buf[klen+1];
    memcpy(buf, key, klen);
    buf[klen] = 0;

    qu_map_member *knode = *qu_find_node(&node->val.map_index.tree, buf);
    if(knode)
        return knode->value;
    return NULL;
}

int qu_get_boolean(qu_ast_node *node, int *value) {
    const char *content = qu_node_content(node);
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

qu_seq_member *qu_seq_iter(qu_ast_node *node) {
    if(node->kind != QU_NODE_SEQUENCE)
        return NULL;
    return TAILQ_FIRST(&node->val.seq_index.items);
}

qu_seq_member *qu_seq_next(qu_seq_member *iter) {
    return TAILQ_NEXT(iter, lst);
}

qu_ast_node *qu_seq_node(qu_seq_member *iter) {
    return iter->value;
}

qu_map_member *qu_map_iter(qu_ast_node *node) {
    if(node->kind != QU_NODE_MAPPING)
        return NULL;
    return TAILQ_FIRST(&node->val.map_index.items);
}

qu_map_member *qu_map_next(qu_map_member *iter) {
    return TAILQ_NEXT(iter, lst);
}

qu_ast_node *qu_map_key(qu_map_member *iter) {
    return iter->key;
}

qu_ast_node *qu_map_value(qu_map_member *iter) {
    return iter->value;
}

void qu_config_array_insert(qu_array_head **head, qu_array_head **tail,
        int *list_size, qu_array_head *member) {
    member->next = NULL;
    if(*tail) {
        (*tail)->next = member;
        *tail = member;
    } else {
        *tail = *head = member;
    }
    *list_size += 1;
}

qu_array_head *qu_config_array_next(qu_array_head *elem) {
    return elem->next;
}

void qu_config_mapping_insert(qu_mapping_head **head, qu_mapping_head **tail,
        int *map_size, qu_mapping_head *member) {
    member->next = NULL;
    if(*tail) {
        (*tail)->next = member;
        *tail = member;
    } else {
        *tail = *head = member;
    }
    *map_size += 1;
}

qu_mapping_head *qu_config_mapping_next(qu_mapping_head *elem) {
    return elem->next;
}

const char *qu_node_tag(qu_ast_node *node) {
    return node->tag;
}
