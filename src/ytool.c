#include <assert.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/queue.h>

#include "yparser.h"
#include "codes.h"
#include "objpath/objpath.h"

char short_options[] = "Ehf:v";
struct option long_options[] = {
    {"extract", 0, NULL, 'E'},
    {"verbose", 0, NULL, 'v'},
    {"expand", 0, NULL, 'e'},
    {"error-basename", 0, NULL, 'r'},
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
    int error_basename;
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
        case 'r':
            options.error_basename = 1;
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

void serialize_yaml(FILE *out, qu_ast_node *node) {
    // temp code, until real serialization implemented
    qu_ast_node *snode = node;
    qu_ast_node *enode = node;
    while(snode && !snode->start_token)
        snode = CIRCLEQ_FIRST(&snode->children);
    while(enode && !enode->end_token)
        enode = CIRCLEQ_LAST(&enode->children);
    if(snode && enode) {
        qu_token *start = snode->start_token;
        qu_token *end = enode->end_token;
        int first_indent = 1;
        int ind = start->indent;
        while(start && start != end) {
            if(start->kind == QU_TOK_INDENT) {
                // The following smells like a hack, but works
                if(start->start_line == start->end_line) {
                    int curind = ind - start->start_char;
                    if(curind < 0)
                        curind = 0;
                    if(curind < start->bytelen) {
                        fwrite(start->data, start->bytelen - curind, 1, out);
                    }
                } else {
                    int curind = ind;
                    if(curind > start->end_char) {
                        curind = start->end_char;
                    }
                    fwrite(start->data, start->bytelen - curind, 1, out);
                }
            } else {
                if(start->start_char-1 < ind) {
                    assert(first_indent);
                    ind = start->start_char-1;
                }
                first_indent = 0;
                fwrite(start->data, start->bytelen, 1, out);
            }
            start = CIRCLEQ_NEXT(start, lst);
        }
        fwrite(start->data, start->bytelen, 1, out);
        fprintf(out, "\n");
    }
}

int extract(char *path, qu_ast_node *root) {
    int res = 1;  // if nothing found
    void *pattern = objpath_compile(path);
    if(!pattern) {
        fprintf(stderr, "Can't compile pattern ``%s''\n", path);
        return 2;
    }
    void *ctx = objpath_start(pattern);
    qu_ast_node *cur = root;
    qu_ast_node *iter = NULL;
    objpath_value_t val;
    int opcode;
    while(objpath_next(ctx, &opcode, &val, (void **)&cur, (void **)&iter)) {
        if(cur->kind == QU_NODE_ALIAS)
            cur = cur->target;
        switch(opcode) {
        case OBJPATH_KEY:
            if(cur->kind != QU_NODE_MAPPING)
                goto fail;
            CIRCLEQ_FOREACH(iter, &cur->children, lst) {
                if(!strcmp(qu_node_content(iter), val.string)) {
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
            if(cur->kind != QU_NODE_SEQUENCE)
                goto fail;
            CIRCLEQ_FOREACH(iter, &cur->children, lst) {
                if(!val.index--) {
                    cur = iter;
                    goto success;
                }
            }
            goto fail;

        case OBJPATH_KEYS:
            if(cur->kind != QU_NODE_MAPPING)
                goto fail;
            iter = CIRCLEQ_FIRST(&cur->children);
            if(!iter)
                goto fail;
            goto success;
        case OBJPATH_VALUES:
            if(cur->kind != QU_NODE_MAPPING)
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
            if(cur->kind != QU_NODE_SEQUENCE)
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

void execute_action(char **argv, qu_ast_node *root) {
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
    qu_init();
    qu_parse_context ctx;
    int rc;
    rc = qu_context_init(&ctx);
    assert(rc != -1);
    rc = qu_load_file(&ctx, options.filename);
    assert(rc != -1);
    rc = qu_tokenize(&ctx);
    assert(rc != -1);
    char *errfn = ctx.filename;
    if(options.error_basename) {
        errfn = strrchr(ctx.filename, '/');
        if(errfn) {
            errfn += 1;
        } else {
            errfn = ctx.filename;
        }
    }
    if(ctx.error_kind) {
        fprintf(stderr, "Error parsing file %s:%d: %s\n",
            errfn, ctx.error_token->start_line,
            ctx.error_text);
        return 1;
    } else {
        //qu_print_tokens(&ctx, stdout);
        //printf("-----------------\n");
        rc = qu_parse(&ctx);
        assert(rc != -1);
        if(ctx.error_kind) {
            fprintf(stderr, "Error parsing file %s:%d: %s\n",
                errfn, ctx.error_token->start_line,
                ctx.error_text);
            return 1;
        }
    }
    execute_action(argv + optind, ctx.document);
    rc = qu_context_free(&ctx);
    assert(rc != -1);
    return 0;
}
