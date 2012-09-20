#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gensource.h"
#include "codes.h"
#include "access.h"
#include "cutil.h"


static int print_default(qu_context_t *ctx, qu_ast_node *node) {
    int rc;
    qu_nodedata *data = node->userdata;
    if(!data)
        return 0;
    if(data->kind == QU_MEMBER_SCALAR) {
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
            printf("(*cfg)%s = %ld;\n", data->expression,
                strtol(qu_node_content(def), NULL, 0));
        } else if(!strncmp((char *)node->tag->data,
                  "!Float", node->tag->bytelen)) {
            printf("(*cfg)%s = %.17f;\n", data->expression,
                strtof(qu_node_content(def), NULL));
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            // TODO(tailhook) escape apropriately
            printf("(*cfg)%s = \"%s\";\n", data->expression,
                qu_node_content(def));
            printf("(*cfg)%s_len = %lu;\n", data->expression,
                strlen(qu_node_content(def)));
        } else if(!strncmp((char *)node->tag->data,
                  "!Dir", node->tag->bytelen)) {
            // TODO(tailhook) escape apropriately
            printf("(*cfg)%s = \"%s\";\n", data->expression,
                qu_node_content(def));
            printf("(*cfg)%s_len = %lu;\n", data->expression,
                strlen(qu_node_content(def)));
        } else if(!strncmp((char *)node->tag->data,
                  "!String", node->tag->bytelen)) {
            // TODO(tailhook) escape apropriately
            printf("(*cfg)%s = \"%s\";\n", data->expression,
                qu_node_content(def));
            printf("(*cfg)%s_len = %lu;\n", data->expression,
                strlen(qu_node_content(def)));
        } else if(!strncmp((char *)node->tag->data,
                  "!Bool", node->tag->bytelen)) {
            int val;
            rc = qu_get_boolean(def, &val);
            assert (rc != -1);
            printf("(*cfg)%s = %d;\n", data->expression, val ? 1 : 0);
        } else {
            assert (0); // Wrong type
        }
    } else if(data->kind == QU_MEMBER_STRUCT) {
        qu_ast_node *key;
        CIRCLEQ_FOREACH(key, &node->children, lst) {
            int rc = print_default(ctx, key->value);
            assert(rc >= 0);
        }
    }
    return 0;
}


static int print_parser(qu_context_t *ctx, qu_ast_node *node, char *name) {
    qu_nodedata *data = node->userdata;
    if(!data)
        return 0;
    if(!ctx->node_vars[ctx->node_level]) {
        ctx->node_vars[ctx->node_level] = 1;
        printf("qu_ast_node *node%d;\n", ctx->node_level);
    }
    if(data->kind == QU_MEMBER_SCALAR) {
        printf("if((node%d = qu_map_get(node%d, \"%s\"))) {\n",
            ctx->node_level, ctx->node_level-1, name);
        if(!strncmp((char *)node->tag->data, "!Int", node->tag->bytelen)) {
            printf("qu_node_to_int(&ctx, node%d, cli.cfg_flags, &(*cfg)%s);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Float", node->tag->bytelen)) {
            printf("qu_node_to_float(&ctx, node%d, cli.cfg_flags, &(*cfg)%s);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            printf("qu_node_to_str(&ctx, node%1$d, cli.cfg_flags, "
                "&(*cfg)%2$s, &(*cfg)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            printf("qu_node_to_str(&ctx, node%1$d, cli.cfg_flags, "
                "&(*cfg)%2$s, &(*cfg)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Dir", node->tag->bytelen)) {
            printf("qu_node_to_str(&ctx, node%1$d, cli.cfg_flags, "
                "&(*cfg)%2$s, &(*cfg)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!String", node->tag->bytelen)) {
            printf("qu_node_to_str(&ctx, node%1$d, cli.cfg_flags, "
                "&(*cfg)%2$s, &(*cfg)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Bool", node->tag->bytelen)) {
            printf("qu_get_boolean(node%d, &(*cfg)%s);\n",
                ctx->node_level, data->expression);
        } else {
            assert (0); // Wrong type
        }
        printf("}\n");
    } else if(node->kind == QU_NODE_MAPPING) {
        printf("if((node%d = qu_map_get(node%d, \"%s\"))) {\n",
                ctx->node_level, ctx->node_level-1, name);

        ctx->node_level += 1;
        ctx->node_vars[ctx->node_level] = 0;
        qu_ast_node *key;
        CIRCLEQ_FOREACH(key, &node->children, lst) {
            int rc = print_parser(ctx, key->value, qu_node_content(key));
            assert(rc >= 0);
        }
        ctx->node_level -= 1;

        printf("}\n");
    }
    return 0;
}

int print_printer(qu_context_t *ctx, qu_ast_node *node) {
    qu_nodedata *data = node->userdata;
    if(!data)
        return 0;
    if(data->kind == QU_MEMBER_SCALAR) {
        if(!strncmp((char *)node->tag->data, "!Int", node->tag->bytelen)) {
            printf("qu_emit_printf(&ctx, NULL, NULL, 0, \"%%ld\", (*cfg)%s);\n",
                data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!UInt", node->tag->bytelen)) {
            printf("qu_emit_printf(&ctx, NULL, NULL, 0, \"%%lu\", (*cfg)%s);\n",
                data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Float", node->tag->bytelen)) {
            printf("qu_emit_printf(&ctx, NULL, NULL, 0, \"%%.17g\", (*cfg)%s);\n",
                data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, 0, (*cfg)%s, -1);\n",
                data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Dir", node->tag->bytelen)) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, 0, (*cfg)%s, -1);\n",
                data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!String", node->tag->bytelen)) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, 0, (*cfg)%s, -1);\n",
                data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Bool", node->tag->bytelen)) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, 0, "
                   "(*cfg)%s ? \"yes\" : \"no\", -1);\n", data->expression);
        } else {
            assert (0); // Wrong type
        }
    } else if(node->kind == QU_NODE_MAPPING) {
        printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_START);\n");
        qu_ast_node *key;
        CIRCLEQ_FOREACH(key, &node->children, lst) {
            printf("qu_emit_scalar(&ctx, NULL, NULL, "
                   "QU_STYLE_PLAIN, \"%s\", -1);\n", qu_node_content(key));
            printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n");
            print_printer(ctx, key->value);
        }
        printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_END);\n");
    }
    return 0;
}


int print_1opt(qu_ast_node *namenode, int arg, int num,
               char *options, int *optlen) {
    if(!namenode)
        return 0;
    int res = 0;
    char *opt = qu_node_content(namenode);
    if(opt) {
        if(opt[1] == '-') { // long option
            printf("{\"%s\", 1, NULL, %d},\n", opt+2, num);
        } else {
            options[(*optlen)++] = opt[1];
            if(arg) {
                options[(*optlen)++] = ':';
            }
        }
        ++res;
    } else if(namenode->kind == QU_NODE_SEQUENCE) {
        qu_ast_node *single;
        CIRCLEQ_FOREACH(single, &namenode->children, lst) {
            opt = qu_node_content(single);
            if(opt && opt[1] == '-') {  // long option
                printf("{\"%s\", %d, NULL, %d},\n", opt+2, arg, num);
            } else {
                options[(*optlen)++] = opt[1];
                if(arg) {
                    options[(*optlen)++] = ':';
                }
            }
            ++res;
        }
    }
    return res;
}

int print_case(qu_ast_node *namenode, int num) {
    if(!namenode)
        return 0;
    int res = 0;
    char *opt = qu_node_content(namenode);
    if(opt) {
        if(opt[1] == '-') { // long option
            printf("case %d:  // %s\n", num, opt);
        } else {
            printf("case '%c':  // %s\n", opt[1], opt);
        }
        ++res;
    } else if(namenode->kind == QU_NODE_SEQUENCE) {
        qu_ast_node *single;
        CIRCLEQ_FOREACH(single, &namenode->children, lst) {
            opt = qu_node_content(single);
            if(opt && opt[1] == '-') {  // long option
                printf("case %d:  // %s\n", num, opt);
            } else {
                printf("case '%c':  // %s\n", opt[1], opt);
            }
            ++res;
        }
    }
    return res;
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
    printf("#include <getopt.h>\n");
    printf("#include <string.h>\n");
    printf("#include <stdint.h>\n");
    printf("\n");
    printf("#include \"%.*s.h\"\n", hlen, header);
    printf("\n");

    ///////////////  config_cli_parse

    printf("int %1$scli_parse(%1$scli_t *cfg, int argc, char **argv) {\n",
        ctx->prefix);
    char options[128] = "hc:D:PC";
    int optlen = strlen(options);
    int optnum = 1000;
    qu_nodedata *data;
    printf("struct option options[] = {\n");
    printf("{\"config\", 1, NULL, 'c'},\n");
    TAILQ_FOREACH(data, &ctx->cli_options, cli_lst) {
        if(print_1opt(qu_map_get(data->node, "command-line"), 1, optnum,
                   options, &optlen))
            ++ optnum;
        if(print_1opt(qu_map_get(data->node, "command-line-incr"), 0, optnum,
                   options, &optlen))
            ++ optnum;
        if(print_1opt(qu_map_get(data->node, "command-line-decr"), 0, optnum,
                   options, &optlen))
            ++ optnum;
        if(print_1opt(qu_map_get(data->node, "command-line-enable"), 0, optnum,
                   options, &optlen))
            ++ optnum;
        if(print_1opt(qu_map_get(data->node, "command-line-disable"), 0,optnum,
                   options, &optlen))
            ++ optnum;
    }
    printf("};\n");
    printf("char *optstr = \"%.*s\";\n", optlen, options);
    printf("int c;\n");
    printf("\n");
    printf("cfg->cfg_filename = \"%s\";\n", ctx->meta.default_config);
    printf("cfg->cfg_flags = QU_FLAGS_VARS;\n");
    printf("cfg->cfg_mode = QU_MODE_NORMAL;\n");

    optnum = 1000;
    printf("while((c = getopt_long(argc, argv, "
           "optstr, options, NULL)) != -1) {\n");
    printf("switch(c) {\n");
    printf("case 'c':\n");
    printf("cfg->cfg_filename = optarg;\n");
    printf("break;\n");
    TAILQ_FOREACH(data, &ctx->cli_options, cli_lst) {
        if(print_case(qu_map_get(data->node, "command-line"), optnum)) {
            ++ optnum;
            printf("break;\n");
        }
        if(print_case(qu_map_get(data->node, "command-line-incr"), optnum)) {
            ++ optnum;
            printf("(*cfg)%s_delta += 1;\n", strrchr(data->expression, '.'));
            printf("(*cfg)%s_delta_set = 1;\n",
                strrchr(data->expression, '.'));
            printf("break;\n");
        }
        if(print_case(qu_map_get(data->node, "command-line-decr"), optnum)) {
            ++ optnum;
            printf("(*cfg)%s_delta -= 1;\n", strrchr(data->expression, '.'));
            printf("(*cfg)%s_delta_set = 1;\n",
                strrchr(data->expression, '.'));
            printf("break;\n");
        }
        if(print_case(qu_map_get(data->node, "command-line-enable"), optnum)) {
            ++ optnum;
            printf("(*cfg)%s = 1;\n", strrchr(data->expression, '.'));
            printf("(*cfg)%s_set = 1;\n", strrchr(data->expression, '.'));
            printf("break;\n");
        }
        if(print_case(qu_map_get(data->node, "command-line-disable"), optnum)){
            ++ optnum;
            printf("(*cfg)%s = 0;\n", strrchr(data->expression, '.'));
            printf("(*cfg)%s_set = 1;\n", strrchr(data->expression, '.'));
            printf("break;\n");
        }
    }
    printf("case '?':\n");
    printf("break;\n");
    printf("}\n");
    printf("}\n");

    printf("return 0;\n");
    printf("}\n");
    printf("\n");

    printf("int %1$scli_apply(%1$smain_t *cfg, %1$scli_t *cli) {\n",
        ctx->prefix);
    TAILQ_FOREACH(data, &ctx->cli_options, cli_lst) {
        int ci = !!qu_map_get(data->node, "command-line");
        int ce = !!qu_map_get(data->node, "command-line-enable");
        int cd = !!qu_map_get(data->node, "command-line-disable");
        if(ci || ce || cd) {
            printf("if((*cli)%s_set) {\n", strrchr(data->expression, '.'));
            printf("(*cfg)%s = (*cli)%s;\n", data->expression,
                   strrchr(data->expression, '.'));
            // TODO(tailhook) set length for strings
            printf("}\n");
        }
        int cinc = !!qu_map_get(data->node, "command-line-incr");
        int cdec = !!qu_map_get(data->node, "command-line-decr");
        if(cinc || cdec) {
            printf("if((*cli)%s_delta_set) {\n",
                strrchr(data->expression, '.'));
            printf("(*cfg)%s += (*cli)%s_delta;\n", data->expression,
                   strrchr(data->expression, '.'));
            qu_ast_node *max = qu_map_get(data->node, "max");
            if(max) {
                // TODO(tailhook) check what kind of a beast the `max` is
                printf("if((*cfg)%1$s > %2$s) (*cfg)%1$s = %2$s;",
                    data->expression, qu_node_content(max));
            }
            qu_ast_node *min = qu_map_get(data->node, "min");
            if(min) {
                // TODO(tailhook) check what kind of a beast the `min` is
                printf("if((*cfg)%1$s < %2$s) (*cfg)%1$s = %2$s;",
                    data->expression, qu_node_content(min));
            }
            // TODO(tailhook) check constraints
            printf("}\n");
        }
    }

    printf("return 0;\n");
    printf("}\n");
    printf("\n");
    ///////////////  config_load

    printf("int %1$sload(%1$smain_t *cfg, int argc, char **argv) {\n",
        ctx->prefix);

    printf("// Setting defaults\n");
    qu_ast_node *key;
    CIRCLEQ_FOREACH(key, &ctx->parsing.document->children, lst) {
        int rc = print_default(ctx, key->value);
        assert(rc >= 0);
    }

    printf("\n");
    printf("// Parsing command-line options\n");
    printf("%scli_t cli;\n", ctx->prefix);
    printf("memset(&cli, 0, sizeof(%scli_t));\n", ctx->prefix);
    printf("%scli_parse(&cli, argc, argv);\n", ctx->prefix);

    printf("\n");
    printf("// Prepare the AST\n");
    printf("int rc;\n");
    printf("qu_parse_context ctx;\n");
    printf("rc = qu_file_parse(&ctx, cli.cfg_filename);\n");
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error parsing_file\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("};\n");

    printf("if(rc > 0) {\n");
    printf("    qu_print_error(&ctx, stderr);\n");
    printf("    exit(126);\n");
    printf("}\n");
    printf("\n");
    printf("qu_ast_node *node0 = qu_get_root(&ctx);\n");

    printf("// Parsing root elements\n");
    ctx->node_level = 1;
    ctx->node_vars[1] = 0;
    CIRCLEQ_FOREACH(key, &ctx->parsing.document->children, lst) {
        int rc = print_parser(ctx, key->value, qu_node_content(key));
        assert(rc >= 0);
    }

    printf("// Overlay command-line options on top\n");
    printf("rc = %scli_apply(cfg, &cli);\n", ctx->prefix);
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error applying command-line args\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("}\n");

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
    printf("return 0;\n");
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
        print_printer(ctx, key->value);
    }
    printf("qu_emit_opcode(&ctx, NULL, NULL, QU_EMIT_MAP_END);\n");

    printf("qu_emit_done(&ctx);\n");
    printf("return 0;\n");
    printf("}\n");

    return 0;
}
