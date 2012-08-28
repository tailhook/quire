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


static int print_parser(qu_context_t *ctx, qu_ast_node *node,
                        char *name, char *varname, char *prefix) {
    if(node->kind == QU_NODE_MAPPING) {
        if(!ctx->node_vars[ctx->node_level]) {
            ctx->node_vars[ctx->node_level] = 1;
            printf("qu_ast_node *node%d;\n", ctx->node_level);
        }
        if(node->tag) {
            printf("if((node%d = qu_map_get(node%d, \"%s\"))) {\n",
                ctx->node_level, ctx->node_level-1, name);
            if(!strncmp((char *)node->tag->data, "!Int", node->tag->bytelen)) {
                printf("%s%s = strtol(qu_node_content(node%d), NULL, 0);\n",
                    prefix, varname, ctx->node_level);
            } else if(!strncmp((char *)node->tag->data,
                      "!UInt", node->tag->bytelen)) {
                printf("%s%s = strtoul(qu_node_content(node%d), NULL, 0);\n",
                    prefix, varname, ctx->node_level);
            } else if(!strncmp((char *)node->tag->data,
                      "!File", node->tag->bytelen)) {
                printf("%s%s = qu_node_content(node%d);\n",
                    prefix, varname, ctx->node_level);
            } else if(!strncmp((char *)node->tag->data,
                      "!File", node->tag->bytelen)) {
                printf("%s%s = qu_node_content(node%d);\n",
                    prefix, varname, ctx->node_level);
            } else if(!strncmp((char *)node->tag->data,
                      "!Dir", node->tag->bytelen)) {
                printf("%s%s = qu_node_content(node%d);\n",
                    prefix, varname, ctx->node_level);
            } else if(!strncmp((char *)node->tag->data,
                      "!String", node->tag->bytelen)) {
                printf("%s%s = qu_node_content(node%d);\n",
                    prefix, varname, ctx->node_level);
            } else if(!strncmp((char *)node->tag->data,
                      "!Bool", node->tag->bytelen)) {
                printf("qu_get_boolean(node%d, &%s%s);\n",
                    ctx->node_level, prefix, varname);
            } else {
                assert (0); // Wrong type
            }
            printf("}\n");
        } else {
            char nprefix[strlen(name) + strlen(prefix) + 2];
            strcpy(nprefix, prefix);
            strcat(nprefix, varname);
            strcat(nprefix, ".");
            printf("if((node%d = qu_map_get(node%d, \"%s\"))) {\n",
                    ctx->node_level, ctx->node_level-1, name);

            ctx->node_level += 1;
            ctx->node_vars[ctx->node_level] = 0;
            qu_ast_node *key;
            CIRCLEQ_FOREACH(key, &node->children, lst) {
                char *mname = qu_c_name(&ctx->parsing.pieces,
                                        qu_node_content(key));
                int rc = print_parser(ctx, key->value, qu_node_content(key),
                                      mname, nprefix);
                assert(rc >= 0);
            }
            ctx->node_level -= 1;

            printf("}\n");
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
    printf("#include <stdio.h>\n");
    printf("#include <errno.h>\n");
    printf("#include <stdlib.h>\n");
    printf("\n");
    printf("#include \"%.*s.h\"\n", hlen, header);
    printf("#include \"yparser.h\"\n");
    printf("#include \"access.h\"\n");
    printf("\n");
    printf("int %1$sload(%1$smain_t *cfg, int argc, char **argv) {\n",
        ctx->prefix);

    printf("// Setting defaults\n");
    qu_ast_node *key;
    CIRCLEQ_FOREACH(key, &ctx->parsing.document->children, lst) {
        char *mname = qu_node_content(key);
        if(!strcmp(mname, "__meta__"))
            continue;
        int rc = print_default(ctx, key->value, mname, "cfg->");
        assert(rc >= 0);
    }

    printf("\n");
    printf("// Parsing command-line options\n");

    printf("\n");
    printf("// Parsing root elements\n");
    printf("qu_init();\n");
    printf("int rc;\n");
    printf("qu_parse_context ctx;\n");
    printf("rc = qu_context_init(&ctx);\n");
    printf("if(rc < 0)\n");
    printf("    return rc;\n");
    printf("rc = qu_load_file(&ctx, \"%s\");\n", ctx->meta.default_config);
    printf("if(rc < 0)\n");
    printf("    return rc;\n");
    printf("rc = qu_tokenize(&ctx);\n");
    printf("if(rc < 0)\n");
    printf("    return rc;\n");

    printf("if(!ctx.error_kind) {\n");
    printf("rc = qu_parse(&ctx);\n");
    printf("if(rc < 0)\n");
    printf("    return rc;\n");
    printf("}\n");

    printf("if(ctx.error_kind) {\n");
    printf("fprintf(stderr, \"Error parsing file %%s:%%d: %%s\\n\",\n");
    printf("\"fn.yaml\", ctx.error_token->start_line,\n");
    printf("ctx.error_text);\n");
    printf("return -EINVAL;\n");
    printf("}\n");
    printf("\n");
    printf("qu_ast_node *node0 = ctx.document;\n");

    ctx->node_level = 1;
    ctx->node_vars[1] = 0;
    CIRCLEQ_FOREACH(key, &ctx->parsing.document->children, lst) {
        char *mname = qu_node_content(key);
        if(!strcmp(mname, "__meta__"))
            continue;
        int rc = print_parser(ctx, key->value, mname, mname, "cfg->");
        assert(rc >= 0);
    }

    printf("\n");
    printf("// Free resources\n");
    printf("qu_context_free(&ctx);\n");

    printf("return 0;\n");
    printf("}\n");
    printf("\n");

    printf("int %1$sfree(%1$smain_t *cfg) {\n", ctx->prefix);
    printf("// TODO\n");
    printf("return 0;");
    printf("}\n");

    return 0;
}
