#ifndef _H_ACCESS
#define _H_ACCESS

#include "yparser.h"

yaml_ast_node *yaml_map_get(yaml_ast_node *node, char *key);

int yaml_get_boolean(yaml_ast_node *node, int *value);

char *yaml_node_content(yaml_ast_node *node);

#endif // _H_ACCESS
