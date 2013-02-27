#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gensource.h"
#include "codes.h"
#include "access.h"
#include "cutil.h"

static void print_string(char *str) {
    putc('"', stdout);
    for(char *c = str; *c; ++c) {
        if(*c < 32) {
            switch(*c) {
            case '\r': printf("\\r"); break;
            case '\n': printf("\\n"); break;
            case '\t': printf("\\t"); break;
            default: printf("\\x%02x", *c); break;
            }
            continue;
        } else if(*c == '\\' || *c == '"') {
            putc('\\', stdout);
            putc(*c, stdout);
        } else {
            putc(*c, stdout);
        }
    }
    putc('"', stdout);
}

static int print_default(qu_context_t *ctx, qu_ast_node *node,
    char *prefix, qu_ast_node *datanode) {
    int rc;
    qu_nodedata *data = node->userdata;
    if(!data)
        return 0;
    if(data->kind == QU_MEMBER_SCALAR) {
        qu_ast_node *def;
        if(datanode->kind == QU_NODE_MAPPING) {
            def = qu_map_get(datanode, "default");
            if(!def)
                def = qu_map_get(datanode, "=");
            if(!def)
                return 0;
        } else {
            def = datanode;
        }

        switch(data->type) {
        case QU_TYP_INT:
            printf("(*cfg)%s%s = %ld;\n", prefix, data->expression,
                strtol(qu_node_content(def), NULL, 0));
            break;
        case QU_TYP_FLOAT:
            printf("(*cfg)%s%s = %.17f;\n", prefix, data->expression,
                strtof(qu_node_content(def), NULL));
            break;
        case QU_TYP_FILE:
            printf("(*cfg)%s%s = ", prefix, data->expression);
            print_string(qu_node_content(def));
            printf(";\n");
            printf("(*cfg)%s%s_len = %lu;\n", prefix, data->expression,
                strlen(qu_node_content(def)));
            break;
        case QU_TYP_DIR:
            printf("(*cfg)%s%s = ", prefix, data->expression);
            print_string(qu_node_content(def));
            printf(";\n");
            printf("(*cfg)%s%s_len = %lu;\n", prefix, data->expression,
                strlen(qu_node_content(def)));
            break;
        case QU_TYP_STRING:
            printf("(*cfg)%s%s = ", prefix, data->expression);
            print_string(qu_node_content(def));
            printf(";\n");
            printf("(*cfg)%s%s_len = %lu;\n", prefix, data->expression,
                strlen(qu_node_content(def)));
            break;
        case QU_TYP_BOOL: {
            int val;
            rc = qu_get_boolean(def, &val);
            assert (rc != -1);
            printf("(*cfg)%s%s = %d;\n", prefix, data->expression,
                val ? 1 : 0);
            } break;
        default:
            assert (0); // Wrong type
        }
    } else if(data->kind == QU_MEMBER_STRUCT) {
        qu_map_member *item;
        TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
            if(node == datanode) {
                int rc = print_default(ctx, item->value,
                                       prefix, item->value);
                assert(rc >= 0);
            } else {
                qu_ast_node *dnode = qu_map_get(datanode,
                    qu_node_content(item->key));
                if(dnode) {
                    int rc = print_default(ctx, item->value,
                                           prefix, dnode);
                    assert(rc >= 0);
                }
            }
        }
    } else if(data->kind == QU_MEMBER_CUSTOM) {
        printf("%1$s%2$s_defaults(&(*cfg)%3$s%4$s);\n",
            ctx->prefix, data->data.custom.typename,
            prefix, data->expression);
        qu_ast_node *defnode = qu_map_get(node, "default");
        if(defnode) {
            qu_ast_node *ctypenode = qu_map_get(
                qu_map_get(ctx->parsing.document, "__types__"),
                data->data.custom.typename);
            assert(ctypenode);
            int rc = print_default(ctx, ctypenode,
                data->expression, defnode);
            assert(rc >= 0);
        }
    }
    return 0;
}


static void print_parser(qu_context_t *ctx, qu_ast_node *node, char *name) {
    qu_nodedata *data = node->userdata;
    if(!data)
        return;
    if(!ctx->node_vars[ctx->node_level]) {
        ctx->node_vars[ctx->node_level] = 1;
        printf("qu_ast_node *node%d;\n", ctx->node_level);
    }
    if(data->kind == QU_MEMBER_SCALAR) {
        printf("if((node%d = qu_map_get(node%d, \"%s\"))) {\n",
            ctx->node_level, ctx->node_level-1, name);
        if(!strncmp((char *)node->tag->data, "!Int", node->tag->bytelen)) {
            printf("qu_node_to_int(ctx, node%d, cli->cfg_flags, &(*cfg)%s);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Float", node->tag->bytelen)) {
            printf("qu_node_to_float(ctx, node%d, cli->cfg_flags, &(*cfg)%s);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            printf("qu_node_to_str(ctx, node%1$d, cli->cfg_flags, "
                "&(*cfg)%2$s, &(*cfg)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            printf("qu_node_to_str(ctx, node%1$d, cli->cfg_flags, "
                "&(*cfg)%2$s, &(*cfg)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Dir", node->tag->bytelen)) {
            printf("qu_node_to_str(ctx, node%1$d, cli->cfg_flags, "
                "&(*cfg)%2$s, &(*cfg)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!String", node->tag->bytelen)) {
            printf("qu_node_to_str(ctx, node%1$d, cli->cfg_flags, "
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
    } else if(data->kind == QU_MEMBER_STRUCT) {
        printf("if((node%d = qu_map_get(node%d, \"%s\"))) {\n",
                ctx->node_level, ctx->node_level-1, name);

        ctx->node_level += 1;
        ctx->node_vars[ctx->node_level] = 0;
        qu_map_member *item;
        TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
            print_parser(ctx, item->value, qu_node_content(item->key));
        }
        ctx->node_level -= 1;

        printf("}\n");
    } else if(data->type == QU_TYP_CUSTOM) {
        printf("if((node%d = qu_map_get(node%d, \"%s\"))) {\n",
                ctx->node_level, ctx->node_level-1, name);

        ctx->node_level += 1;
        ctx->node_vars[ctx->node_level] = 0;
        printf("%1$s%2$s_parse(&(*cfg)%3$s, node%4$d, ctx, cli);\n",
            ctx->prefix, data->data.custom.typename,
            data->expression, ctx->node_level-1);
        ctx->node_level -= 1;

        printf("}\n");
    } else if(data->type == QU_TYP_ARRAY) {
        // TODO(tailhook)
    } else if(data->type == QU_TYP_MAP) {
        // TODO(tailhook)
    } else {
        assert(0);
    }
}

int print_printer(qu_context_t *ctx, qu_ast_node *node) {
    qu_nodedata *data = node->userdata;
    if(!data)
        return 0;
    if(data->kind == QU_MEMBER_SCALAR) {
        switch(data->type) {
        case QU_TYP_INT:
            printf("qu_emit_printf(ctx, NULL, NULL, 0, \"%%ld\", (*cfg)%s);\n",
                data->expression);
            break;
        case QU_TYP_FLOAT:
            printf("qu_emit_printf(ctx, NULL, NULL, 0, \"%%.17g\", (*cfg)%s);\n",
                data->expression);
            break;
        case QU_TYP_FILE:
            printf("qu_emit_scalar(ctx, NULL, NULL, 0, (*cfg)%s, -1);\n",
                data->expression);
            break;
        case QU_TYP_DIR:
            printf("qu_emit_scalar(ctx, NULL, NULL, 0, (*cfg)%s, -1);\n",
                data->expression);
            break;
        case QU_TYP_STRING:
            printf("qu_emit_scalar(ctx, NULL, NULL, 0, (*cfg)%s, -1);\n",
                data->expression);
            break;
        case QU_TYP_BOOL:
            printf("qu_emit_scalar(ctx, NULL, NULL, 0, "
                   "(*cfg)%s ? \"yes\" : \"no\", -1);\n", data->expression);
            break;
        default:
            assert(0);
        }
    } else if(data->kind == QU_MEMBER_STRUCT) {
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_START);\n");
        qu_map_member *item;
        TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
            if(item->value->userdata) {
                printf("qu_emit_scalar(ctx, NULL, NULL, "
                       "QU_STYLE_PLAIN, \"%s\", -1);\n",
                       qu_node_content(item->key));
                printf("qu_emit_opcode(ctx, "
                       "NULL, NULL, QU_EMIT_MAP_VALUE);\n");
                print_printer(ctx, item->value);
            }
        }
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n");
    } else if(data->type == QU_TYP_CUSTOM) {
        printf("%1$s%2$s_print(&(*cfg)%3$s, ctx);\n", ctx->prefix,
            data->data.custom.typename, data->expression);
    } else if(data->type == QU_TYP_ARRAY) {
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_START);\n");
        // TODO(tailhook)
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_END);\n");
    } else if(data->type == QU_TYP_MAP) {
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_START);\n");
        // TODO(tailhook)
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n");
    } else {
        assert(0);
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
        qu_seq_member *item;
        TAILQ_FOREACH(item, &namenode->val.seq_index.items, lst) {
            opt = qu_node_content(item->value);
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
        qu_seq_member *item;
        TAILQ_FOREACH(item, &namenode->val.seq_index.items, lst) {
            opt = qu_node_content(item->value);
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
    printf("{NULL, 1, NULL, 0}\n");
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


    // type-specific functions
    qu_map_member *item;
    qu_ast_node *types = qu_map_get(ctx->parsing.document, "__types__");
    if(types) {
        qu_map_member *typ;
        TAILQ_FOREACH(typ, &types->val.map_index.items, lst) {

            // set defaults
            printf("void %1$s%2$s_defaults(%1$s%2$s_t *cfg) {\n",
                ctx->prefix, qu_node_content(typ->key));
            TAILQ_FOREACH(item, &typ->value->val.map_index.items, lst) {
                print_default(ctx, item->value, "", item->value);
            }
            printf("}\n");
            printf("\n");

            // parse
            printf("void %1$s%2$s_parse(%1$s%2$s_t *cfg, qu_ast_node *node0,"
                   "qu_parse_context *ctx, %1$scli_t *cli) {\n",
                ctx->prefix, qu_node_content(typ->key));
            ctx->node_level = 1;
            ctx->node_vars[1] = 0;
            TAILQ_FOREACH(item, &typ->value->val.map_index.items, lst) {
                print_parser(ctx, item->value, qu_node_content(item->key));
            }
            printf("}\n");
            printf("\n");

            // print
            printf("void %1$s%2$s_print(%1$s%2$s_t *cfg, "
                   "qu_emit_context *ctx) {\n",
                ctx->prefix, qu_node_content(typ->key));
            printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_START);\n");

            TAILQ_FOREACH(item, &typ->value->val.map_index.items, lst) {
                if(item->value->userdata) {
                    char *mname = qu_node_content(item->key);
                    printf("qu_emit_scalar(ctx, NULL, NULL, "
                           "QU_STYLE_PLAIN, \"%s\", -1);\n", mname);
                    printf("qu_emit_opcode(ctx, "
                           "NULL, NULL, QU_EMIT_MAP_VALUE);\n");
                    print_printer(ctx, item->value);
                }
            }

            printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n");
            printf("}\n");
            printf("\n");

        }
    }

    ///////////////  config_load

    printf("int %1$sload(%1$smain_t *cfg, int argc, char **argv) {\n",
        ctx->prefix);

    printf("// Setting defaults\n");
    TAILQ_FOREACH(item, &ctx->parsing.document->val.map_index.items, lst) {
        print_default(ctx, item->value, "", item->value);
    }

    printf("\n");
    printf("// Parsing command-line options\n");
    printf("%scli_t ccli;\n", ctx->prefix);
    printf("%scli_t *cli = &ccli;\n", ctx->prefix);
    printf("memset(cli, 0, sizeof(%scli_t));\n", ctx->prefix);
    printf("%scli_parse(cli, argc, argv);\n", ctx->prefix);

    printf("\n");
    printf("// Prepare the AST\n");
    printf("int rc;\n");
    printf("qu_parse_context cctx;\n");
    printf("qu_parse_context *ctx = &cctx;\n");
    printf("rc = qu_file_parse(ctx, cli->cfg_filename);\n");
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error parsing_file\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("};\n");

    printf("if(rc > 0) {\n");
    printf("    qu_print_error(ctx, stderr);\n");
    printf("    exit(126);\n");
    printf("}\n");
    printf("\n");
    printf("rc = qu_process_includes(ctx, QU_IFLAG_FROMFILE"
        "|QU_IFLAG_INCLUDE|QU_IFLAG_GLOBSEQ|QU_IFLAG_GLOBMAP);\n");
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error parsing_file\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("}\n");
    printf("rc = qu_merge_maps(ctx, QU_MFLAG_MAPMERGE"
        "|QU_MFLAG_SEQMERGE|QU_MFLAG_RESOLVEALIAS);\n");
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error parsing_file\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("}\n");
    printf("\n");
    printf("qu_ast_node *node0 = qu_get_root(ctx);\n");

    printf("// Parsing root elements\n");
    ctx->node_level = 1;
    ctx->node_vars[1] = 0;
    TAILQ_FOREACH(item, &ctx->parsing.document->val.map_index.items, lst) {
        print_parser(ctx, item->value, qu_node_content(item->key));
    }

    printf("// Overlay command-line options on top\n");
    printf("rc = %scli_apply(cfg, cli);\n", ctx->prefix);
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error applying command-line args\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("}\n");

    printf("\n");
    printf("// Free resources\n");
    printf("qu_context_free(ctx);\n");

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
    printf("qu_emit_context cctx;\n");
    printf("qu_emit_context *ctx = &cctx;\n");
    printf("qu_emit_init(ctx, stream);\n");
    printf("\n");
    printf("qu_emit_comment(ctx, 0, \"Program name: %s\", -1);\n",
        ctx->meta.program_name);
    printf("qu_emit_whitespace(ctx, QU_WS_ENDLINE, 1);\n");
    printf("\n");

    printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_START);\n");
    TAILQ_FOREACH(item, &ctx->parsing.document->val.map_index.items, lst) {
        if(item->value->userdata) {
            char *mname = qu_node_content(item->key);
            printf("qu_emit_scalar(ctx, NULL, NULL, "
                   "QU_STYLE_PLAIN, \"%s\", -1);\n", mname);
            printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n");
            print_printer(ctx, item->value);
        }
    }
    printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n");

    printf("qu_emit_done(ctx);\n");
    printf("return 0;\n");
    printf("}\n");

    return 0;
}
