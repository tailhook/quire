#ifndef _H_ACCESS
#define _H_ACCESS

#include "node.h"
#include "parser.h"
#include "../quire_int.h"

typedef struct qu_array_head {
    struct qu_array_head *next;
} qu_array_head __attribute__((aligned(sizeof(void*)*2)));
typedef struct qu_mapping_head {
    struct qu_mapping_head *next;
} qu_mapping_head __attribute__((aligned(sizeof(void*)*2)));


qu_map_member **qu_find_node(qu_map_member **root, const char *value);
const char *qu_parse_content(qu_ast_node *node, struct obstack *buf);

qu_ast_node *qu_map_get_len(qu_ast_node *node, const char *key, int klen);


// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
qu_ast_node *qu_map_get(qu_ast_node *node, const char *key);
qu_seq_member *qu_seq_iter(qu_ast_node *node);
qu_seq_member *qu_seq_next(qu_seq_member *iter);
qu_ast_node *qu_seq_node(qu_seq_member *iter);
qu_map_member *qu_map_iter(qu_ast_node *node);
qu_map_member *qu_map_next(qu_map_member *iter);
qu_ast_node *qu_map_key(qu_map_member *iter);
qu_ast_node *qu_map_value(qu_map_member *iter);
const char *qu_node_tag(qu_ast_node *node);
int qu_get_boolean(qu_ast_node *node, int *value);
const char *qu_node_content(qu_ast_node *node);

#endif // _H_ACCESS
