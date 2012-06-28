#include <assert.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/queue.h>

#include "yparser.h"
#include "objpath/objpath.h"

char short_options[] = "Ehf:v";
struct option long_options[] = {
    {"extract", 0, NULL, 'E'},
    {"verbose", 0, NULL, 'v'},
    {"help", 0, NULL, 'h'},
    {"filename", 1, NULL, 'f'},
    {NULL, 0, NULL, 0}
    };
struct {
    enum {
        A_HELP,
        A_EXTRACT
    } action;
    int verbosity;
    char *filename;
} options;


void print_usage(FILE *stream) {
    fprintf(stream,
        "Usage:\n"
        "  ytool {-E --extract} [options] [arguments...]\n"
        "\n"
        "Actions:\n"
        "  -E, --extract [key1 [key2 ...]]\n"
        "            Extract keys from yaml file\n"
        "\n"
        "Options:\n"
        "  -v, --verbose\n"
        "            Be more verbose (always print keys)\n"
        "  -f, --input FILE\n"
        "            Input filename (stdin by default)\n"
        "\n"
        );
}

void parse_options(int argc, char **argv) {
    int opt;
    while((opt = getopt_long(argc, argv, short_options, long_options, NULL))) {
        switch(opt) {
        case -1:
            return;
        case 'E':
            options.action = A_EXTRACT;
            break;
        case 'v':
            options.verbosity += 1;
            break;
        case 'f':
            options.filename = optarg;
            break;
        case 'h':
            options.action = A_HELP;
            print_usage(stdout);
            exit(0);
        default:
            print_usage(stderr);
            exit(99);
        }
    }
}

void serialize_yaml(FILE *out, yaml_ast_node *node) {
    yaml_token *start = node->start_token;
    yaml_token *end = node->end_token;
    while(start && start != end) {
        fwrite(start->data, start->bytelen, 1, out);
        start = CIRCLEQ_NEXT(start, lst);
    }
    fwrite(start->data, start->bytelen, 1, out);
    fprintf(out, "\n");
}

char *scalar_node_value(yaml_ast_node *node) {
    if(node->content)
        return node->content;
    if(node->start_token == node->end_token
        && node->start_token->kind == TOKEN_PLAINSTRING) {
        node->content = strndup(node->start_token->data,
                                node->start_token->bytelen);
        return node->content;
    }
    fprintf(stderr, "Not implemented\n");
    exit(98);
}

int extract(char *path, yaml_ast_node *root) {
    int res = 1;  // if nothing found
    void *pattern = objpath_compile(path);
    void *ctx = objpath_start(pattern);
    yaml_ast_node *cur = root;
    yaml_ast_node *iter = NULL;
    objpath_value_t val;
    int opcode;
    while(objpath_next(ctx, &opcode, &val, (void **)&cur, (void **)&iter)) {
        switch(opcode) {
        case OBJPATH_KEY:
            if(cur->kind != NODE_MAPPING)
                goto fail;
            CIRCLEQ_FOREACH(iter, &cur->children, lst) {
                if(!strcmp(scalar_node_value(iter), val.string)) {
                    cur = CIRCLEQ_NEXT(iter, lst);
                    goto success;
                } else {
                    // each second is a key
                    iter = CIRCLEQ_NEXT(iter, lst);
                    if(!iter) break;
                }
            }
            goto fail;
        case OBJPATH_INDEX:
            if(cur->kind != NODE_SEQUENCE)
                goto fail;
            CIRCLEQ_FOREACH(iter, &cur->children, lst) {
                if(!val.index--) {
                    cur = iter;
                    goto success;
                }
            }
            goto fail;

        case OBJPATH_KEYS:
            if(cur->kind != NODE_MAPPING)
                goto fail;
            iter = CIRCLEQ_FIRST(&cur->children);
            if(!iter)
                goto fail;
            goto success;
        case OBJPATH_VALUES:
            if(cur->kind != NODE_MAPPING)
                goto fail;
            iter = CIRCLEQ_FIRST(&cur->children);
            if(!iter)
                goto fail;
            // the second element is the first value
            iter = CIRCLEQ_NEXT(iter, lst);
            if(!iter)
                goto fail;
            goto success;
        case OBJPATH_NEXTKEY:
        case OBJPATH_NEXTVALUE:
            iter = CIRCLEQ_NEXT(iter, lst);
            if(!iter)
                goto fail;
            // each second is a key or value
            iter = CIRCLEQ_NEXT(iter, lst);
            if(!iter)
                goto fail;
            goto success;

        case OBJPATH_ELEMENTS:
            if(cur->kind != NODE_SEQUENCE)
                goto fail;
            iter = CIRCLEQ_FIRST(&cur->children);
            if(!iter)
                goto fail;
            goto success;
        case OBJPATH_NEXTELEMENT:
            iter = CIRCLEQ_NEXT(iter, lst);
            if(!iter)
                goto fail;
            goto success;

        case OBJPATH_FINAL:
            serialize_yaml(stdout, cur);
            res = 0;  // at least one element found
            goto success;
        }
        success:
            continue;
        fail:
            cur = NULL;
            continue;
    }
    objpath_free(ctx);
    return res;
}

void execute_action(char **argv, yaml_ast_node *root) {
    if(options.action == A_EXTRACT) {
        exit(extract(argv[0], root));
    } else {
        fprintf(stderr, "Not implemented\n");
        exit(98);
    }
}

int main(int argc, char **argv) {
    parse_options(argc, argv);
    assert(argc >= 2);
    yaml_init();
    yaml_parse_context ctx;
    int rc;
    rc = yaml_context_init(&ctx);
    assert(rc != -1);
    rc = yaml_load_file(&ctx, options.filename);
    assert(rc != -1);
    rc = yaml_tokenize(&ctx);
    assert(rc != -1);
    if(ctx.error_kind) {
        fprintf(stderr, "Error parsing file %s:%d: %s\n",
            ctx.filename, ctx.error_token->start_line,
            ctx.error_text);
    } else {
        //yaml_print_tokens(&ctx, stdout);
        //printf("-----------------\n");
        rc = yaml_parse(&ctx);
        assert(rc != -1);
    }
    execute_action(argv + optind, ctx.document);
    rc = yaml_context_free(&ctx);
    assert(rc != -1);
    return 0;
}
