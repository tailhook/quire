#include <stdio.h>

#include "metadata.h"
#include "access.h"

int parse_metadata(yaml_ast_node *root, coyaml_metadata_t *meta) {
    meta->program_name = NULL;
    meta->default_config = NULL;
    meta->description = NULL;
    meta->has_arguments = 0;
    meta->mixed_arguments = 0;
    yaml_ast_node *mnode = yaml_map_get(root, "__meta__");
    if(!mnode)
        return 0;

    yaml_ast_node *tnode;
    char *tdata;

    tnode = yaml_map_get(mnode, "program-name");
    if(tnode) {
        tdata = yaml_node_content(tnode);
        if(!tdata) {
            fprintf(stderr, "__meta__.program-name must be scalar");
            return -1;
        }
        meta->program_name = tdata;
    }

    tnode = yaml_map_get(mnode, "default-config");
    if(tnode) {
        tdata = yaml_node_content(tnode);
        if(!tdata) {
            fprintf(stderr, "__meta__.default-config[ must be scalar");
            return -1;
        }
        meta->default_config = tdata;
    }

    tnode = yaml_map_get(mnode, "description");
    if(tnode) {
        tdata = yaml_node_content(tnode);
        if(!tdata) {
            fprintf(stderr, "__meta__.description must be scalar");
            return -1;
        }
        meta->description = tdata;
    }

    tnode = yaml_map_get(mnode, "has-arguments");
    if(tnode) {
        int value;
        if(yaml_get_boolean(tnode, &value) == -1) {
            fprintf(stderr, "__meta__.has-arguments must be boolean");
            return -1;
        }
        meta->has_arguments = value;
    }

    tnode = yaml_map_get(mnode, "mixed-arguments");
    if(tnode) {
        int value;
        if(yaml_get_boolean(tnode, &value) == -1) {
            fprintf(stderr, "__meta__.mixed-arguments must be boolean");
            return -1;
        }
        if(value && !meta->has_arguments) {
            fprintf(stderr, "__meta__.mixed-arguments without has-arguments"
                " is useless");
            return -1;
        }
        meta->mixed_arguments = value;
    }

    return 0;
}