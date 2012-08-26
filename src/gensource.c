#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gensource.h"
#include "codes.h"
#include "access.h"
#include "cutil.h"


static int print_default(qu_context_t *ctx, qu_ast_node *node,
                         char *name, char *prefix) {
    int rc;
    if(node->kind == QU_NODE_MAPPING) {
        if(node->tag) {

            qu_ast_node *def = qu_map_get(node, "default");
            if(!def)
                def = qu_map_get(node, "=");
            if(!def)
                return 0;

            if(!strncmp((char *)node->tag->data, "!Int", node->tag->bytelen)) {
                printf("%s%s = %ld;\n", prefix, name,
                    strtol(qu_node_content(def), NULL, 0));
            } else if(!strncmp((char *)node->tag->data,
                      "!UInt", node->tag->bytelen)) {
                printf("%s%s = %lu;\n", prefix, name,
                    strtoul(qu_node_content(def), NULL, 0));
            } else if(!strncmp((char *)node->tag->data,
                      "!File", node->tag->bytelen)) {
                printf("%s%s = \"%s\";\n", prefix, name,
                    qu_node_content(def));
            } else if(!strncmp((char *)node->tag->data,
                      "!Dir", node->tag->bytelen)) {
                printf("%s%s = \"%s\";\n", prefix, name,
                    qu_node_content(def));
            } else if(!strncmp((char *)node->tag->data,
                      "!String", node->tag->bytelen)) {
                printf("%s%s = \"%s\";\n", prefix, name,
                    qu_node_content(def));
            } else if(!strncmp((char *)node->tag->data,
                      "!Bool", node->tag->bytelen)) {
                int val;
                rc = qu_get_boolean(def, &val);
                assert (rc != -1);
                printf("%s%s = %d;\n", prefix, name, val ? 1 : 0);
            } else {
                assert (0); // Wrong type
            }
        } else {
            char nprefix[strlen(name) + strlen(prefix) + 2];
            strcpy(nprefix, prefix);
            strcat(nprefix, name);
            strcat(nprefix, ".");
            qu_ast_node *key;
            CIRCLEQ_FOREACH(key, &node->children, lst) {
                char *mname = qu_c_name(&ctx->parsing.pieces,
                                        qu_node_content(key));
                int rc = print_default(ctx, key->value, mname, nprefix);
                assert(rc >= 0);
            }
        }
    }
    return 0;
}


int qu_output_source(qu_context_t *ctx) {
    char *header = ctx->options.output_source;
    char *tmp = strrchr(header, '/');
    if(tmp) {
        header = tmp + 1;
    }
    int hlen;
    tmp = strchr(header, '.');
    if(tmp) {
        hlen = tmp - header;
    } else {
        hlen = strlen(header);
    }
    printf("/* DO NOT EDIT THIS FILE! */\n");
    printf("/* This file is generated */\n");
    printf("#include \"%.*s.h\"\n", hlen, header);
    printf("\n");
    printf("int %1$sload(%1$smain_t *cfg, int argc, char **argv) {\n",
        ctx->prefix);

    qu_ast_node *key;
    CIRCLEQ_FOREACH(key, &ctx->parsing.document->children, lst) {
        char *mname = qu_node_content(key);
        if(!strcmp(mname, "__meta__"))
            continue;
        int rc = print_default(ctx, key->value, mname, "cfg->");
        assert(rc >= 0);
    }

    printf("}\n");
    printf("\n");
    return 0;
}
