#ifndef _H_ACCESS
#define _H_ACCESS

#include "yparser.h"

struct qu_array_head {
	struct qu_array_head *next;
} qu_array_head __attribute__((aligned(sizeof(void*)*2)));


qu_map_member **qu_find_node(qu_map_member **root, char *value);


// PUBLIC API
// Keep in sync with quire.h
// Think about ABI compatibility
qu_ast_node *qu_get_root(qu_parse_context *ctx);
qu_ast_node *qu_map_get(qu_ast_node *node, char *key);
int qu_get_boolean(qu_ast_node *node, int *value);
char *qu_node_content(qu_ast_node *node);

#endif // _H_ACCESS
