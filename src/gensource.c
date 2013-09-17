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
        qu_ast_node *cnode = qu_map_get(node, "__value__");
        if(cnode && cnode->userdata) {
            print_default(ctx, cnode, prefix, datanode);
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


static void print_parser(qu_context_t *ctx, qu_ast_node *node);

static void print_parse_member(qu_context_t *ctx,
    qu_ast_node *node, char *name) {
    qu_nodedata *data = node->userdata;
    if(!data)
        return;
    if(!ctx->node_vars[ctx->node_level]) {
        ctx->node_vars[ctx->node_level] = 1;
        printf("qu_ast_node *node%d;\n", ctx->node_level);
    }
    printf("if((node%d = qu_map_get(node%d, \"%s\"))) {\n",
        ctx->node_level, ctx->node_level-1, name);
    print_parser(ctx, node);
    printf("}\n");
}

static void print_parser(qu_context_t *ctx, qu_ast_node *node) {
    qu_nodedata *data = node->userdata;
    assert(data);
    if(data->kind == QU_MEMBER_SCALAR) {
        if(!strncmp((char *)node->tag->data, "!Int", node->tag->bytelen)) {
            printf("qu_node_to_int(ctx, node%d, cli->cfg_flags, &(*targ)%s);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Float", node->tag->bytelen)) {
            printf("qu_node_to_float(ctx, node%d, cli->cfg_flags, &(*targ)%s);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            printf("qu_node_to_str(ctx, node%1$d, cli->cfg_flags, "
                "&(*targ)%2$s, &(*targ)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!File", node->tag->bytelen)) {
            printf("qu_node_to_str(ctx, node%1$d, cli->cfg_flags, "
                "&(*targ)%2$s, &(*targ)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Dir", node->tag->bytelen)) {
            printf("qu_node_to_str(ctx, node%1$d, cli->cfg_flags, "
                "&(*targ)%2$s, &(*targ)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!String", node->tag->bytelen)) {
            printf("qu_node_to_str(ctx, node%1$d, cli->cfg_flags, "
                "&(*targ)%2$s, &(*targ)%2$s_len);\n",
                ctx->node_level, data->expression);
        } else if(!strncmp((char *)node->tag->data,
                  "!Bool", node->tag->bytelen)) {
            printf("qu_get_boolean(node%d, &(*targ)%s);\n",
                ctx->node_level, data->expression);
        } else {
            assert (0); // Wrong type
        }
    } else if(data->kind == QU_MEMBER_STRUCT) {
        ctx->node_level += 1;
        ctx->node_vars[ctx->node_level] = 0;
        qu_map_member *item;
        TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
            print_parse_member(ctx, item->value, qu_node_content(item->key));
        }
        ctx->node_level -= 1;
    } else if(data->type == QU_TYP_CUSTOM) {
        ctx->node_level += 1;
        ctx->node_vars[ctx->node_level] = 0;
        printf("%1$s%2$s_parse(cfg, &(*targ)%3$s, node%4$d, ctx, cli);\n",
            ctx->prefix, data->data.custom.typename,
            data->expression, ctx->node_level-1);
        ctx->node_level -= 1;

    } else if(data->type == QU_TYP_ARRAY) {
        printf("for(qu_seq_member *seq%1$d = qu_seq_iter(node%2$d);"
               " seq%1$d; seq%1$d = qu_seq_next(seq%1$d)) {\n",
               ctx->node_level+1, ctx->node_level);
        printf("%1$sa_%2$s_t *el = qu_config_alloc(cfg, "
            "sizeof(%1$sa_%2$s_t));\n",
            ctx->prefix, data->data.array.membername);
        printf("{\n");
        printf("%1$sa_%2$s_t *targ = el;\n",
            ctx->prefix, data->data.array.membername);
        ctx->node_level += 1;
        printf("qu_ast_node *node%1$d = qu_seq_node(seq%1$d);\n",
            ctx->node_level);
        qu_ast_node *eltype = qu_map_get(node, "element");
        qu_nodedata *eldata = eltype->userdata;
        if(eldata->type == QU_TYP_CUSTOM) {
            printf("%1$s%2$s_defaults(&(*targ)%3$s);\n",
                ctx->prefix, eldata->data.custom.typename,
                eldata->expression);
        }
        print_parser(ctx, eltype);
        ctx->node_level -= 1;
        printf("}\n");
        printf("qu_config_array_insert("
            "(void **)&(*targ)%1$s, (void **)&(*targ)%1$s_last, "
            "&(*targ)%1$s_len, &el->head);\n",
            data->expression);
        printf("}\n");
    } else if(data->type == QU_TYP_MAP) {
        printf("for(qu_map_member *mem%1$d = qu_map_iter(node%2$d);"
               " mem%1$d; mem%1$d = qu_map_next(mem%1$d)) {\n",
               ctx->node_level+1, ctx->node_level);
        printf("%1$sm_%2$s_%3$s_t *el = qu_config_alloc(cfg, "
            "sizeof(%1$sm_%2$s_%3$s_t));\n", ctx->prefix,
            data->data.mapping.keyname, data->data.mapping.valuename);
        printf("{\n");
        printf("%1$sm_%2$s_%3$s_t *targ = el;\n", ctx->prefix,
            data->data.mapping.keyname, data->data.mapping.valuename);
        ctx->node_level += 1;

        printf("qu_ast_node *node%1$d = qu_map_key(mem%1$d);\n",
            ctx->node_level);
        qu_ast_node *ktype = qu_map_get(node, "key-element");
        qu_nodedata *kdata = ktype->userdata;
        assert(kdata->kind == QU_MEMBER_SCALAR);
        print_parser(ctx, ktype);

        printf("node%1$d = qu_map_value(mem%1$d);\n", ctx->node_level);
        qu_ast_node *vtype = qu_map_get(node, "value-element");
        qu_nodedata *vdata = vtype->userdata;
        if(vdata->type == QU_TYP_CUSTOM) {
            printf("%1$s%2$s_defaults(&(*targ)%3$s);\n",
                ctx->prefix, vdata->data.custom.typename,
                vdata->expression);
        }
        print_parser(ctx, vtype);

        ctx->node_level -= 1;
        printf("}\n");
        printf("qu_config_mapping_insert("
            "(void **)&(*targ)%1$s, (void **)&(*targ)%1$s_last, "
            "&(*targ)%1$s_len, &el->head);\n",
            data->expression);
        printf("}\n");
    } else {
        assert(0);
    }
}

int print_printer(qu_context_t *ctx, qu_ast_node *node, char *tag) {
    qu_nodedata *data = node->userdata;
    if(!data)
        return 0;
    if(data->kind == QU_MEMBER_SCALAR) {
        switch(data->type) {
        case QU_TYP_INT:
            printf("qu_emit_printf(ctx, %s, NULL, 0, \"%%ld\", (*cfg)%s);\n",
                tag, data->expression);
            break;
        case QU_TYP_FLOAT:
            printf("qu_emit_printf(ctx, %s, NULL, 0, \"%%.17g\", (*cfg)%s);\n",
                tag, data->expression);
            break;
        case QU_TYP_FILE:
            printf("qu_emit_scalar(ctx, %s, NULL, 0, (*cfg)%s, -1);\n",
                tag, data->expression);
            break;
        case QU_TYP_DIR:
            printf("qu_emit_scalar(ctx, %s, NULL, 0, (*cfg)%s, -1);\n",
                tag, data->expression);
            break;
        case QU_TYP_STRING:
            printf("qu_emit_scalar(ctx, %s, NULL, 0, (*cfg)%s, -1);\n",
                tag, data->expression);
            break;
        case QU_TYP_BOOL:
            printf("qu_emit_scalar(ctx, %s, NULL, 0, "
                   "(*cfg)%s ? \"yes\" : \"no\", -1);\n",
                   tag, data->expression);
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
                print_printer(ctx, item->value, "NULL");
            }
        }
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n");
    } else if(data->type == QU_TYP_CUSTOM) {
        printf("%1$s%2$s_print(&(*cfg)%3$s, ctx);\n", ctx->prefix,
            data->data.custom.typename, data->expression);
    } else if(data->type == QU_TYP_ARRAY) {
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_START);\n");
        printf("for(%1$sa_%2$s_t *elem=(*cfg)%3$s; elem; "
               "elem = qu_config_array_next(elem)) {\n",
            ctx->prefix, data->data.array.membername, data->expression);
        printf("{\n");
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_ITEM);\n");
        printf("%1$sa_%2$s_t *cfg = elem;\n",
            ctx->prefix, data->data.array.membername);
        print_printer(ctx, qu_map_get(node, "element"), "NULL");
        printf("}\n");
        printf("}\n");
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_END);\n");
    } else if(data->type == QU_TYP_MAP) {
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_START);\n");
        printf("for(%1$sm_%2$s_%3$s_t *elem=(*cfg)%4$s; elem; "
               "elem = qu_config_array_next(elem)) {\n", ctx->prefix,
                data->data.mapping.keyname, data->data.mapping.valuename,
                data->expression);
        printf("{\n");
        printf("%1$sm_%2$s_%3$s_t *cfg = elem;\n", ctx->prefix,
            data->data.mapping.keyname, data->data.mapping.valuename);
        print_printer(ctx, qu_map_get(node, "key-element"), "NULL");
        printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);\n");
        print_printer(ctx, qu_map_get(node, "value-element"), "NULL");
        printf("}\n");
        printf("}\n");
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

    printf("int %1$scli_parse(qu_parse_context *ctx, %1$scli_t *cfg,"
           "int argc, char **argv) {\n", ctx->prefix);
    char options[128] = "hc:D:PC";
    int optlen = strlen(options);
    int optnum = 1000;
    qu_nodedata *data;
    printf("struct option options[] = {\n");
    printf("{\"config\", 1, NULL, 'c'},\n");
    printf("{\"config-var\", 1, NULL, 'D'},\n");
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
    printf("case 'D': {\n");
    printf("char *eq = strchr(optarg, '=');\n");
    printf("if(eq) {\n");
    printf("*eq = 0;\n");
    printf("qu_set_string(ctx, optarg, eq+1);\n");
    printf("*eq = '=';\n");
    printf("} else {\n");
    printf("qu_set_string(ctx, optarg, \"\");\n");
    printf("}\n");
    printf("}; break;\n");
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
            switch(data->type) {
            case QU_TYP_FILE:
            case QU_TYP_DIR:
            case QU_TYP_STRING:
                printf("(*cfg)%s_len = strlen((*cli)%s);\n", data->expression,
                       strrchr(data->expression, '.'));
                break;
            }
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
            qu_ast_node *cnode = qu_map_get(typ->value, "__value__");
            qu_ast_node *tnode = qu_map_get(typ->value, "__tags__");
            char *typname = qu_node_content(typ->key);
            char *pname = NULL, *defname = NULL;

            // set defaults
            printf("void %1$s%2$s_defaults(%1$s%2$s_t *cfg) {\n",
                ctx->prefix, typname);
            if(tnode) {
                qu_ast_node *prop = qu_map_get(tnode, "__property__");
                qu_ast_node *def = qu_map_get(tnode, "__default__");
                if(def) {
                    defname = qu_node_content(def);
                    pname = prop ? qu_node_content(prop) : "tag";
                    printf("(*cfg).%s = %s;\n", pname,
                        (char*)qu_map_get(tnode, defname)->userdata);
                }
            }
            TAILQ_FOREACH(item, &typ->value->val.map_index.items, lst) {
                print_default(ctx, item->value, "", item->value);
            }
            printf("}\n");
            printf("\n");

            if(tnode) {
                printf("int %1$s%2$s_tag_to_int(char *tag, int len) {\n",
                    ctx->prefix, typname);
                qu_map_member *item;
                TAILQ_FOREACH(item, &tnode->val.map_index.items, lst) {
                    if(qu_node_content(item->key)[0] == '_')
                        continue;
                    printf("if(len == %d && !strncmp(\"!%s\", tag, len))\n",
                        (int)strlen(qu_node_content(item->key))+1,
                        qu_node_content(item->key));
                    printf("return %s;\n", (char *)item->value->userdata);
                }
                printf("return -1;\n");
                printf("}\n");
                printf("\n");

                printf("char *%1$s%2$s_int_to_tag(int value, int skip_def) {\n",
                    ctx->prefix, typname);
                printf("switch(value) {\n");
                TAILQ_FOREACH(item, &tnode->val.map_index.items, lst) {
                    if(qu_node_content(item->key)[0] == '_')
                        continue;
                    printf("case %s:\n", (char *)item->value->userdata);
                    if(defname && !strcmp(
                        qu_node_content(item->key), defname)) {
                        printf("return skip_def ? NULL : \"%s\";\n",
                            qu_node_content(item->value));
                    } else {
                        printf("return \"!%s\";\n",
                            qu_node_content(item->key));
                    }
                }
                printf("default:\n");
                printf("return NULL;\n");
                printf("}\n");
                printf("}\n");
                printf("\n");

            }

            char *conversion = NULL;
            if(cnode && cnode->tag->bytelen == 8 &&
                !strncmp((char *)cnode->tag->data,
                         "!Convert", cnode->tag->bytelen)) {
                conversion = qu_node_content(cnode);
                printf("// Forward declaration\n");
                printf("void %3$s(qu_parse_context *ctx, %1$smain_t *cfg, "
                    "%1$s%2$s_t *targ, char *value);\n",
                    ctx->prefix, typname, conversion);
            }

            // parse
            ctx->node_level = 0;
            ctx->node_vars[0] = 1;
            printf("void %1$s%2$s_parse(%1$smain_t *cfg,"
                   " %1$s%2$s_t *targ, "
                   "qu_ast_node *node0, "
                   "qu_parse_context *ctx, %1$scli_t *cli) {\n",
                ctx->prefix, typname);
            if(tnode) {
                printf("char *tag;\n");
                printf("int taglen;\n");
                printf("qu_get_tag(node0, &tag, &taglen);\n");
                printf("if(tag) {\n");
                printf("(*targ).%s = %s%s_tag_to_int(tag, taglen);\n",
                       pname, ctx->prefix, typname);
                printf("if((*targ).%s == -1)\n", pname);
                printf("qu_report_error(ctx, node0, \"Wrong tag\");\n");
                printf("}\n");
            }
			if(conversion) {
				printf("if(qu_node_content(node0)) {\n");
				printf("%s(ctx, cfg, targ, qu_node_content(node0));\n",
					conversion);
				printf("}\n");
			} else if(cnode) {
				printf("if(qu_node_content(node0) || qu_seq_iter(node0)) {\n");
                print_parser(ctx, cnode);
				printf("}\n");
            }
            ctx->node_level = 1;
            ctx->node_vars[1] = 0;
            TAILQ_FOREACH(item, &typ->value->val.map_index.items, lst) {
                char *mname = qu_node_content(item->key);
                if(mname && *mname == '_')
                    continue;
                print_parse_member(ctx, item->value,
                    qu_node_content(item->key));
            }
            printf("}\n");
            printf("\n");

            // print
            printf("void %1$s%2$s_print(%1$s%2$s_t *cfg, "
                   "qu_emit_context *ctx) {\n",
                ctx->prefix, typname);
            int first_member = 1;
            char tagbuf[128];
            if(tnode) {
                snprintf(tagbuf, 128, "%s%s_int_to_tag((*cfg).%s, 1)",
                         ctx->prefix, typname, pname);
            } else {
                sprintf(tagbuf, "NULL");
            }
            tagbuf[127] = 0;
            TAILQ_FOREACH(item, &typ->value->val.map_index.items, lst) {
                if(item->value->userdata) {
                    char *mname = qu_node_content(item->key);
                    if(mname && *mname == '_')
                        continue;
                    if(first_member) {
                        printf("qu_emit_opcode(ctx, %s, NULL, "
                            "QU_EMIT_MAP_START);\n", tagbuf);
                        first_member = 0;
                    }
                    printf("qu_emit_scalar(ctx, NULL, NULL, "
                           "QU_STYLE_PLAIN, \"%s\", -1);\n", mname);
                    printf("qu_emit_opcode(ctx, "
                           "NULL, NULL, QU_EMIT_MAP_VALUE);\n");
                    print_printer(ctx, item->value, "NULL");
                }
            }
            if(first_member) {
                if(cnode && !conversion) {
                    print_printer(ctx, cnode, tagbuf);
                } else {
                    printf("qu_emit_opcode(ctx, %s, NULL, "
                        "QU_EMIT_MAP_START);\n", tagbuf);
                    printf("qu_emit_opcode(ctx, NULL, NULL, "
                        "QU_EMIT_MAP_END);\n");
                }
            } else {
                if(cnode && !conversion) {
                    printf("qu_emit_scalar(ctx, NULL, NULL, "
                           "QU_STYLE_PLAIN, \"=\", -1);\n");
                    printf("qu_emit_opcode(ctx, "
                           "NULL, NULL, QU_EMIT_MAP_VALUE);\n");
                    print_printer(ctx, cnode, "NULL");
                }
                printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n");
            }
            printf("}\n");
            printf("\n");

        }
    }

    ///////////////  config_set_defaults

    printf("int %1$sset_defaults(%1$smain_t *cfg) {\n", ctx->prefix);
    TAILQ_FOREACH(item, &ctx->parsing.document->val.map_index.items, lst) {
        print_default(ctx, item->value, "", item->value);
    }

    printf("return 0;\n");
    printf("}\n");
    printf("\n");


	///////////////  config_do_parse
    printf("int %1$sdo_parse(qu_parse_context *ctx, %1$scli_t *cli, "
		"%1$smain_t *cfg) {\n", ctx->prefix);

    printf("\n");
    printf("// Prepare the AST\n");
    printf("int rc;\n");
    printf("rc = qu_file_parse(ctx, cli->cfg_filename);\n");
    printf("if(rc < 0) {\n");
    printf("    qu_print_error(ctx, stderr);\n");
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
    printf("    qu_print_error(ctx, stderr);\n");
    printf("    exit(127);\n");
    printf("}\n");
    printf("rc = qu_merge_maps(ctx, QU_MFLAG_MAPMERGE"
        "|QU_MFLAG_SEQMERGE|QU_MFLAG_RESOLVEALIAS);\n");
    printf("if(rc < 0) {\n");
    printf("    qu_print_error(ctx, stderr);\n");
    printf("    exit(127);\n");
    printf("}\n");
    printf("\n");
    printf("qu_ast_node *node0 = qu_get_root(ctx);\n");
    printf("%smain_t *targ = cfg; // same for root element\n", ctx->prefix);

    printf("// Parsing root elements\n");
    ctx->node_level = 1;
    ctx->node_vars[1] = 0;
    TAILQ_FOREACH(item, &ctx->parsing.document->val.map_index.items, lst) {
        print_parse_member(ctx, item->value, qu_node_content(item->key));
    }
    printf("return 0;\n");
    printf("}\n");
    printf("\n");


    ///////////////  config_load

    printf("int %1$sload(%1$smain_t *cfg, int argc, char **argv) {\n",
        ctx->prefix);
	printf("int rc;\n");

    printf("qu_config_init(cfg, sizeof(*cfg));\n");
    printf("%1$sset_defaults(cfg);\n", ctx->prefix);

    printf("\n");
    printf("// Prepare context\n");
    printf("qu_parse_context cctx;\n");
    printf("qu_parse_context *ctx = &cctx;\n");
    printf("qu_parser_init(ctx);\n");
    printf("\n");

    printf("// Parsing command-line options\n");
    printf("%scli_t ccli;\n", ctx->prefix);
    printf("%scli_t *cli = &ccli;\n", ctx->prefix);
    printf("memset(cli, 0, sizeof(%scli_t));\n", ctx->prefix);
    printf("%scli_parse(ctx, cli, argc, argv);\n", ctx->prefix);

	printf("%1$sdo_parse(ctx, cli, cfg);\n", ctx->prefix);

    printf("// Overlay command-line options on top\n");
    printf("rc = %scli_apply(cfg, cli);\n", ctx->prefix);
    printf("if(rc < 0) {\n");
    printf("    perror(\"%s: libquire: Error applying command-line args\");\n",
        ctx->meta.program_name);
    printf("    exit(127);\n");
    printf("}\n");

    printf("\n");
    printf("// Free resources\n");
    printf("qu_parser_free(ctx);\n");

    // Temporary
    printf("%1$sprint(cfg, 0, stdout);\n", ctx->prefix);

    printf("return 0;\n");
    printf("}\n");
    printf("\n");

    ///////////////  config_free

    printf("int %1$sfree(%1$smain_t *cfg) {\n", ctx->prefix);
    printf("qu_config_free(cfg);\n");
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
            print_printer(ctx, item->value, "NULL");
        }
    }
    printf("qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);\n");

    printf("qu_emit_done(ctx);\n");
    printf("return 0;\n");
    printf("}\n");

    return 0;
}
