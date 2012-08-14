#ifndef _H_ACCESS
#define _H_ACCESS

#include "yparser.h"


qu_ast_node **qu_find_node(qu_ast_node **root, char *value);


qu_ast_node *qu_map_get(qu_ast_node *node, char *key);

int qu_get_boolean(qu_ast_node *node, int *value);

char *qu_node_content(qu_ast_node *node);

#endif // _H_ACCESS
