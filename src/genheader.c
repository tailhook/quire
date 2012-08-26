#include <sys/queue.h>
#include <stdio.h>
#include <assert.h>

#include "genheader.h"
#include "yparser.h"
#include "codes.h"
#include "cutil.h"


struct scalar_type_s {
    char *tag;
    char *typ;
} scalar_types[] = {
    {"!Int", "long"},
    {"!UInt", "unsigned long"},
    {"!File", "char*"},
    {"!Dir", "char*"},
    {"!String", "char*"},
    {"!Bool", "int"},
    {NULL, NULL}
    };


static int print_member(qu_context_t *ctx, qu_ast_node *node, char *name) {
    if(node->kind == QU_NODE_MAPPING) {
        if(node->tag) {
            struct scalar_type_s *st = scalar_types;
            for(;st->tag;++st) {
                if(!strncmp((char *)node->tag->data, st->tag,
                            node->tag->bytelen)) {
                    printf("%s %s;\n", st->typ, name);
                    return 0;
                }
            }
            return -1;
        } else {
            qu_ast_node *key;
            CIRCLEQ_FOREACH(key, &node->children, lst) {
                char *mname = qu_c_name(&ctx->parsing.pieces,
                                        qu_node_content(key));
                int rc = print_member(ctx, key->value, mname);
                assert(rc >= 0);
            }
        }
    }

    return 0;
}


int qu_output_header(qu_context_t *ctx) {
    printf("/* DO NOT EDIT THIS FILE! */\n");
    printf("/* This file is generated */\n");
    printf("#ifndef _H_%s\n", ctx->macroprefix);
    printf("#define _H_%s\n", ctx->macroprefix);
    printf("\n");

    // TODO describe array|mapping element structures

    printf("typedef %smain_s {\n", ctx->prefix);

    qu_ast_node *key;
    CIRCLEQ_FOREACH(key, &ctx->parsing.document->children, lst) {
        char *mname = qu_node_content(key);
        if(!strcmp(mname, "__meta__"))
            continue;
        int rc = print_member(ctx, key->value, mname);
        assert(rc >= 0);
    }

    printf("} %smain_t;\n", ctx->prefix);

    printf("\n");  //end of types

    printf("int %1$sload(%1$smain_t *cfg, int argc, char **argv);\n",
        ctx->prefix);

    printf("\n");
    printf("#endif  // _H_%s\n", ctx->macroprefix);
    return 0;
}
