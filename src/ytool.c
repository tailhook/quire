#include <assert.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/queue.h>

#include "yparser.h"
#include "error.h"
#include "access.h"
#include "codes.h"
#include "emitter.h"
#include "../objpath/objpath.h"
#include "maputil.h"

char short_options[] = "Ehf:vkp";
struct option long_options[] = {
    {"extract", 0, NULL, 'E'},
    {"verbose", 0, NULL, 'v'},
    {"plain", 0, NULL, 'p'},
    {"keep-formatting", 0, NULL, 'k'},
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
    int keep_formatting;
    int plain;
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
        "  -k, --keep-formatting\n"
        "            Keep formatting of original file "
                    "(incl. comments and anchors)\n"
        "  -p, --plain\n"
        "            Process plain structure (merge maps, resolve anchors..)\n"
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
        case 'k':
            options.keep_formatting = 1;
            break;
        case 'p':
            options.plain = 1;
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

static void _emit_node(qu_emit_context *ctx, qu_ast_node *node) {
    switch(node->kind) {
    case QU_NODE_MAPPING: {
        qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_START);
        qu_map_member *item;
        TAILQ_FOREACH(item, &node->val.map_index.items, lst) {
            _emit_node(ctx, item->key);
            qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_VALUE);
            _emit_node(ctx, item->value);
        }
        qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_MAP_END);
        } break;
    case QU_NODE_SEQUENCE: {
        qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_START);
        qu_seq_member *item;
        TAILQ_FOREACH(item, &node->val.seq_index.items, lst) {
            qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_ITEM);
            _emit_node(ctx, item->value);
        }
        qu_emit_opcode(ctx, NULL, NULL, QU_EMIT_SEQ_END);
        } break;
    case QU_NODE_SCALAR:
        qu_emit_scalar(ctx, NULL, NULL, QU_STYLE_PLAIN,
                       qu_node_content(node), -1);
        break;
    case QU_NODE_ALIAS: {
        char alias[node->start_token->bytelen+1];
        alias[node->start_token->bytelen] = 0;
        memcpy(alias, node->start_token->data, node->start_token->bytelen);
        qu_emit_alias(ctx, alias);
        } break;
    default:
        assert(0);
    }
}

void emit_yaml(FILE *out, qu_ast_node *node) {
    qu_emit_context ctx;
    qu_emit_init(&ctx, out);
    _emit_node(&ctx, node);
    qu_emit_done(&ctx);
}

void serialize_yaml(FILE *out, qu_ast_node *node) {
    // temp code, until real serialization implemented
    qu_ast_node *snode = node;
    qu_ast_node *enode = node;
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
            cur = cur->val.alias_target;
        switch(opcode) {
        case OBJPATH_KEY:
            if(cur->kind != QU_NODE_MAPPING)
                goto fail;
            cur = qu_map_get(cur, val.string);
            if(cur)
                goto success;
            goto fail;
        case OBJPATH_INDEX: {
            if(cur->kind != QU_NODE_SEQUENCE)
                goto fail;
            qu_seq_member *item;
            TAILQ_FOREACH(item, &cur->val.seq_index.items, lst) {
                if(!val.index--) {
                    cur = item->value;
                    goto success;
                }
            }
            } goto fail;
        case OBJPATH_ELEMENTS:
            if(cur->kind != QU_NODE_SEQUENCE)
                goto fail;
            assert(0); // not implemented
            //iter = TAILQ_FIRST(&cur->val.seq_);
            if(!iter)
                goto fail;
            goto success;
        case OBJPATH_NEXTELEMENT:
            assert(0); // not implemented
            //iter = CIRCLEQ_NEXT(iter, lst);
            if(!iter)
                goto fail;
            goto success;

        case OBJPATH_FINAL:
            if(options.keep_formatting) {
                serialize_yaml(stdout, cur);
            } else {
                emit_yaml(stdout, cur);
            }
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
    qu_parse_context ctx;
    int rc;
    rc = qu_file_parse(&ctx, options.filename);
    if(rc == 1) {
        qu_print_error(&ctx, stderr);
        return 1;
    } else if(rc < 0) {
        fprintf(stderr, "quire-gen: Error parsing \"%s\": %s\n",
            options.filename, strerror(-rc));
        return 1;
    }
    if(options.plain) {
        _qu_merge_maps(&ctx,
            QU_MFLAG_MAPMERGE|QU_MFLAG_SEQMERGE|QU_MFLAG_RESOLVEALIAS);
    }
    execute_action(argv + optind, ctx.document);
    qu_context_free(&ctx);
    return 0;
}
