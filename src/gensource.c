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
    if(node->tag) {
        qu_ast_node *def;
        if(node->kind == QU_NODE_MAPPING) {
            def = qu_map_get(node, "default");
            if(!def)
                def = qu_map_get(node, "=");
            if(!def)
                return 0;
        } else {
            def = node;
        }

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
    } else if(node->kind == QU_NODE_MAPPING) {
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
    return 0;
}


static int print_parser(qu_context_t *ctx, qu_ast_node *node,
                        char *name, char *varname, char *prefix) {
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
    } else if(node->kind == QU_NODE_MAPPING) {
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
    return 0;
}

int print_printer(qu_context_t *ctx, qu_ast_node *node,
                  char *name, char *prefix) {
    if(node->tag) {
        if(!strncmp((char *)node->tag->data, "!Int", node->tag->bytelen)) {
            printf("qu_emit_printf(&ctx, NULL, NULL, 0, \"%%ld\", %s%s);\n",
                prefix, name);
        } else if(!strncmp((char *)node->tag->data,
                  "!UInt", node->tag->bytelen)) {
            printf("qu_emit_printf(&ctx, NULL, NULL, 0, \"%%lu\", %s%s);\n",
                prefix, name);
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, 0, %s%s, -1);\n",
                prefix, name);
        } else if(!strncmp((char *)node->tag->data,
                  "!Dir", node->tag->bytelen)) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, 0, %s%s, -1);\n",
                prefix, name);
        } else if(!strncmp((char *)node->tag->data,
                  "!String", node->tag->bytelen)) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, 0, %s%s, -1);\n",
                prefix, name);
        } else if(!strncmp((char *)node->tag->data,
                  "!Bool", node->tag->bytelen)) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, 0, "
                   "%s%s ? \"yes\" : \"no\", -1);\n", prefix, name);
        } else {
            assert (0); // Wrong type
        }
    } else if(node->kind == QU_NODE_MAPPING) {
        char nprefix[strlen(name) + strlen(prefix) + 2];
        strcpy(nprefix, prefix);
        strcat(nprefix, name);
        strcat(nprefix, ".");
        printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_START);\n");
        qu_ast_node *key;
        CIRCLEQ_FOREACH(key, &node->children, lst) {
            char *mname = qu_c_name(&ctx->parsing.pieces,
                                    qu_node_content(key));
            printf("qu_emit_scalar(&ctx, NULL, NULL, "
                   "QU_STYLE_PLAIN, \"%s\", -1);\n", qu_node_content(key));
            printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n");
            print_printer(ctx, key->value, mname, nprefix);
        }
        printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_END);\n");
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
    printf("\n");

    ///////////////  config_load

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
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error creating context\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("};\n");
    printf("rc = qu_load_file(&ctx, \"%s\");\n", ctx->meta.default_config);
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error reading file\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("};\n");
    printf("rc = qu_tokenize(&ctx);\n");
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error tokenizing\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("};\n");

    printf("if(!qu_has_error(&ctx)) {\n");
    printf("rc = qu_parse(&ctx);\n");
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error parsing\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("}\n");
    printf("}\n");

    printf("if(qu_has_error(&ctx)) {\n");
    printf("    qu_print_error(&ctx, stderr);\n");
    printf("    exit(126);\n");
    printf("}\n");
    printf("\n");
    printf("qu_ast_node *node0 = qu_get_root(&ctx);\n");

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

    // Temporary
    printf("%1$sprint(cfg, 0, stdout);\n", ctx->prefix);

    printf("return 0;\n");
    printf("}\n");
    printf("\n");

    ///////////////  config_free

    printf("int %1$sfree(%1$smain_t *cfg) {\n", ctx->prefix);
    printf("// TODO\n");
    printf("return 0;");
    printf("}\n");
    printf("\n");

    ///////////////  config_print

    printf("int %1$sprint(%1$smain_t *cfg, int flags, FILE *stream) {\n",
        ctx->prefix);
    printf("qu_emit_context ctx;\n");
    printf("qu_emit_init(&ctx, stream);\n");
    printf("\n");
    printf("qu_emit_comment(&ctx, 0, \"Program name: %s\", -1);\n",
        ctx->meta.program_name);
    printf("qu_emit_whitespace(&ctx, QU_WS_ENDLINE, 1);\n");
    printf("\n");

    printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_START);\n");
    CIRCLEQ_FOREACH(key, &ctx->parsing.document->children, lst) {
        char *mname = qu_node_content(key);
        if(!strcmp(mname, "__meta__"))
            continue;
        printf("qu_emit_scalar(&ctx, NULL, NULL, "
               "QU_STYLE_PLAIN, \"%s\", -1);\n", mname);
        printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n");
        print_printer(ctx, key->value, mname, "cfg->");
    }
    printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_END);\n");

    printf("qu_emit_done(&ctx);\n");
    printf("return 0;");
    printf("}\n");

    return 0;
}
