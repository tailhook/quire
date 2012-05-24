#include <assert.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/queue.h>

#include "yparser.h"

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

void execute_action(char **argv, yaml_ast_node *root) {
    if(options.action == A_EXTRACT) {
        int resultok = 0;
        if(!strcmp(argv[0], ".")) {
            serialize_yaml(stdout, root);
        } else if(argv[0][0] != '.') {
            fprintf(stderr, "Path must start with a dot\n");
            exit(99);
        } else {
            yaml_ast_node *curnode = root;
            if(isdigit(argv[0][1])) {
                if(curnode->kind != NODE_SEQUENCE) {
                    exit(1); // Nothing found
                }
                int idx = atoi(argv[0]+1);
                yaml_ast_node *child;
                CIRCLEQ_FOREACH(child, &curnode->children, lst) {
                    if(!idx--) {
                        resultok = 1;
                        serialize_yaml(stdout, child);
                    }
                }
            } else {
                if(curnode->kind != NODE_MAPPING) {
                    exit(1); // Nothing found
                }
                yaml_ast_node *child;
                CIRCLEQ_FOREACH(child, &curnode->children, lst) {
                    if(!strcmp(scalar_node_value(child), argv[0]+1)) {
                        resultok = 1;
                        child = CIRCLEQ_NEXT(child, lst);
                        serialize_yaml(stdout, child);
                    } else {
                        child = CIRCLEQ_NEXT(child, lst);
                        if(!child) break;
                    }
                }
            }
        }
        if(!resultok) exit(1);
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
