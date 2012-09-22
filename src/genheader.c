#include <sys/queue.h>
#include <stdio.h>
#include <assert.h>

#include "genheader.h"
#include "yparser.h"
#include "codes.h"
#include "cutil.h"
#include "access.h"

#define STRING_TYPE(typ) ((typ) == QU_TYP_FILE \
                          || (typ) == QU_TYP_DIR \
                          || (typ) == QU_TYP_STRING)


static char *typenames[] = {
    [QU_TYP_INT] = "long",
    [QU_TYP_FLOAT] = "double",
    [QU_TYP_FILE] = "char *",
    [QU_TYP_DIR] = "char *",
    [QU_TYP_STRING] = "char *",
    [QU_TYP_BOOL] = "int",
    [QU_TYP_CUSTOM] = NULL,
    [QU_TYP_ARRAY] = NULL,
    [QU_TYP_MAP] = NULL,
    };


static void print_member(qu_context_t *ctx, qu_ast_node *node) {
    qu_nodedata *data = node->userdata;
    if(!data)
        return;
    if(data->kind == QU_MEMBER_SCALAR) {
        char *memname = strrchr(data->expression, '.')+1;
        printf("%s %s;\n", typenames[data->type], memname);
        if(STRING_TYPE(data->type)) {
            printf("size_t %s_len;\n", memname);
        }
        return;
    } else if(data->kind == QU_MEMBER_STRUCT) {
        printf("struct {\n");
        qu_ast_node *key;
        CIRCLEQ_FOREACH(key, &node->children, lst) {
            print_member(ctx, key->value);
        }
        printf("} %s;\n", strrchr(data->expression, '.')+1);
        return;
    }
    // TODO(tailhook) print other members
}


int qu_output_header(qu_context_t *ctx) {
    printf("/* DO NOT EDIT THIS FILE! */\n");
    printf("/* This file is generated */\n");
    printf("#ifndef _H_%s\n", ctx->macroprefix);
    printf("#define _H_%s\n", ctx->macroprefix);
    printf("\n");
    printf("#include <quire.h>\n");
    printf("\n");

    // TODO describe array|mapping element structures

    printf("typedef struct %scli_s {\n", ctx->prefix);
    printf("char *cfg_filename;\n");
    printf("uint64_t cfg_flags;\n");
    printf("int cfg_mode;\n");
    qu_nodedata *data;
    TAILQ_FOREACH(data, &ctx->cli_options, cli_lst) {
        printf("unsigned %s_set:1;\n", data->cli_name);
    }
    TAILQ_FOREACH(data, &ctx->cli_options, cli_lst) {
        if(data->kind != QU_MEMBER_SCALAR)
            assert(0);  // non scalar command-line not supported
        printf("%s %s;\n", typenames[data->type], data->cli_name);
        if(qu_map_get(data->node, "command-line-incr")
           || qu_map_get(data->node, "command-line-decr")) {
            printf("%s %s_delta;\n", typenames[data->type],
                data->cli_name);
            printf("unsigned %s_delta_set;\n", data->cli_name);
        }
    }
    printf("} %scli_t;\n", ctx->prefix);
    printf("\n");

    qu_ast_node *types = qu_map_get(ctx->parsing.document, "__types__");
    if(types) {
        qu_ast_node *typ;
        CIRCLEQ_FOREACH(typ, &types->children, lst) {
            printf("typedef struct %s%s_s {\n",
                ctx->prefix, qu_node_content(typ));
            qu_ast_node *key;
            CIRCLEQ_FOREACH(key, &typ->value->children, lst) {
                print_member(ctx, key->value);
            }
            printf("} %s%s_t;\n", ctx->prefix, qu_node_content(typ));
            printf("\n");
        }
    }

    printf("typedef struct %smain_s {\n", ctx->prefix);
    qu_ast_node *key;
    CIRCLEQ_FOREACH(key, &ctx->parsing.document->children, lst) {
        print_member(ctx, key->value);
    }
    printf("} %smain_t;\n", ctx->prefix);

    printf("\n");  //end of types

    printf("int %1$sload(%1$smain_t *cfg, int argc, char **argv);\n",
        ctx->prefix);
    printf("int %1$sprint(%1$smain_t *cfg, int flags, FILE *);\n",
        ctx->prefix);
    printf("int %1$sfree(%1$smain_t *cfg);\n", ctx->prefix);

    printf("\n");
    printf("#endif  // _H_%s\n", ctx->macroprefix);
    return 0;
}
