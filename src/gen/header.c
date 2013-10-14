#include <sys/queue.h>
#include <stdio.h>
#include <assert.h>

#include "header.h"
#include "../yaml/parser.h"
#include "../yaml/codes.h"
#include "util/name.h"
#include "util/print.h"
#include "../yaml/access.h"

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
        qu_map_member *item;
        TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
            print_member(ctx, item->value);
        }
        printf("} %s;\n", strrchr(data->expression, '.')+1);
        return;
    } else if(data->kind == QU_MEMBER_CUSTOM) {
        printf("struct %s%s_s %s;\n",
            ctx->prefix, data->data.custom.typename,
            strrchr(data->expression, '.')+1);
    } else if(data->kind == QU_MEMBER_ARRAY) {
		printf("struct %sa_%s_s *%s;\n",
			ctx->prefix, data->data.array.membername,
            strrchr(data->expression, '.')+1);
		printf("struct %sa_%s_s *%s_last;\n",
			ctx->prefix, data->data.array.membername,
            strrchr(data->expression, '.')+1);
		printf("int %s_len;\n", strrchr(data->expression, '.')+1);
    } else if(data->kind == QU_MEMBER_MAP) {
		printf("struct %sm_%s_%s_s *%s;\n",
			ctx->prefix,
			data->data.mapping.keyname,
			data->data.mapping.valuename,
            strrchr(data->expression, '.')+1);
		printf("struct %sm_%s_%s_s *%s_last;\n",
			ctx->prefix,
			data->data.mapping.keyname,
			data->data.mapping.valuename,
            strrchr(data->expression, '.')+1);
		printf("int %s_len;\n", strrchr(data->expression, '.')+1);
	}
    // TODO(tailhook) print other members
}


int qu_output_header(qu_context_t *ctx) {
    qu_code_print(ctx,
        "/*  DO NOT EDIT THIS FILE!  */\n"
        "/*  This file is generated  */\n"
        "#ifndef HEADER_${mpref}\n"
        "#define HEADER_${mpref}\n"
        "\n"
        "#include <quire.h>\n"
        "\n"
        , NULL);

	qu_code_print(ctx,
        "\n"
        "/*  Forward declarations  */\n"
        "\n"
        , NULL);

    qu_fwdecl_print_all(ctx);

	qu_code_print(ctx,
        "\n"
        "/*  Intermediate command-line storage  */\n"
        "\n"
        , NULL);
    qu_cli_print_struct(ctx);

    /*
    qu_ast_node *types = NULL; // TODO(tailhook) remove the stub
    if(types) {
        qu_map_member *typ;
        TAILQ_FOREACH(typ, &types->val.map_index.items, lst) {
            qu_ast_node *tags = qu_map_get(typ->value, "__tags__");
            if(tags && !strcmp(tags->userdata, qu_node_content(typ->key))) {
                printf("typedef enum %s%s_tag_e {\n",
                    ctx->prefix, qu_node_content(typ->key));
                qu_map_member *item;
                TAILQ_FOREACH(item, &tags->val.map_index.items, lst) {
                    if(qu_node_content(item->key)[0] == '_')
                        continue;
                    printf("%s = %d",
                        (char *)item->value->userdata,
                        atoi(qu_node_content(item->value)));
                    if(TAILQ_NEXT(item, lst))
                        printf(",");
                    printf("\n");
                }
                printf("} %s%s_tag_t;\n",
                    ctx->prefix, qu_node_content(typ->key));
                printf("\n");
            }
            printf("typedef struct %s%s_s {\n",
                ctx->prefix, qu_node_content(typ->key));
            if(tags) {
                qu_ast_node *prop = qu_map_get(tags, "__property__");
                if(prop) {
                    printf("%s%s_tag_t %s;\n", ctx->prefix,
                        (char *)tags->userdata, qu_node_content(prop));
                } else {
                    printf("%s%s_tag_t tag;\n",
                        ctx->prefix, (char *)tags->userdata);
                }
            }
            qu_map_member *item;
            TAILQ_FOREACH(item, &typ->value->val.map_index.items, lst) {
                print_member(ctx, item->value);
            }
            printf("} %s%s_t;\n", ctx->prefix, qu_node_content(typ->key));
            printf("\n");
        }
    }

	TAILQ_FOREACH(aitem, &ctx->arrays, data.array.lst) {
		printf("typedef struct %sa_%s_s {\n",
			ctx->prefix, aitem->data.array.membername);
		printf("qu_array_head head;\n");
		print_member(ctx, qu_map_get(aitem->node, "element"));
		printf("} %sa_%s_t;\n", ctx->prefix,
			aitem->data.array.membername);
		printf("\n");
	}
	TAILQ_FOREACH(aitem, &ctx->mappings, data.mapping.lst) {
		printf("typedef struct %sm_%s_%s_s {\n", ctx->prefix,
			aitem->data.mapping.keyname, aitem->data.mapping.valuename);
		printf("qu_mapping_head head;");
		print_member(ctx, qu_map_get(aitem->node, "key-element"));
		print_member(ctx, qu_map_get(aitem->node, "value-element"));
		printf("} %sm_%s_%s_t;\n", ctx->prefix,
			aitem->data.mapping.keyname, aitem->data.mapping.valuename);
		printf("\n");
	}
    */

	qu_code_print(ctx,
        "\n"
        "/*  Main configuration structure  */\n"
        "\n"
        , NULL);

    qu_code_print(ctx,
        "struct ${pref}_main {\n"
        "    qu_config_head head;\n"
        , NULL);
    qu_map_member *item;
    TAILQ_FOREACH(item, &ctx->parser.document->val.map_index.items, lst) {
        print_member(ctx, item->value);
    }
    qu_code_print(ctx,
        "};\n"
        "\n",
        NULL);

    qu_cli_print_fwdecl(ctx);
    qu_code_print(ctx,
        "int ${pref}_load(struct ${pref}_main *cfg, int argc, char **argv);\n"
        "int ${pref}_parse(struct qu_config_context *ctx, struct ${pref}_cli *cli, "
                          "struct ${pref}_main *cfg);\n"
        "int ${pref}_set_defaults(struct ${pref}_main *cfg);\n"
        "void ${pref}_print(struct ${pref}_main *cfg, int flags, FILE *);\n"
        "void ${pref}_help(FILE *);\n"
        "int ${pref}_free(struct ${pref}_main *cfg);\n"
        "\n"
        "#endif  /*  HEADER_${mpref}  */\n"
        , NULL);
    return 0;
}
