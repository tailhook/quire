#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>

#include "include.h"
#include "fromfile.h"
#include "globseq.h"
#include "globmap.h"
#include "merge.h"
#include "template.h"
#include "eval.h"
#include "vars.h"
#include "../yaml/codes.h"
#include "../yaml/access.h"

typedef qu_ast_node *(*qu_directive_processor)(struct qu_parser *ctx,
                                              qu_ast_node *source);

static struct qu_tag_entry {
    const char *tag;
    unsigned flag;
    qu_directive_processor processor;
} qu_tag_registry[] = {
    {"!Include", QU_RAW_FLAG_INCLUDE, qu_raw_include},
    {"!FromFile", QU_RAW_FLAG_INCLUDE, qu_raw_fromfile},
    {"!GlobSeq", QU_RAW_FLAG_INCLUDE, qu_raw_globseq},
    {"!GlobMap", QU_RAW_FLAG_INCLUDE, qu_raw_globmap}
};

const int qu_tag_registry_len =
    sizeof(qu_tag_registry)/sizeof(qu_tag_registry[0]);



const char *qu_join_filenames(struct qu_parser *ctx,
    const char *base, const char *target)
{
    if(!target)
        return NULL;
    if(*target == '/')
        return target;
    const char *slash = strrchr(base, '/');
    if(!slash)
        return target;
    obstack_blank(&ctx->pieces, 0);
    obstack_grow(&ctx->pieces, base, slash - base + 1);
    obstack_grow0(&ctx->pieces, target, strlen(target));
    return obstack_finish(&ctx->pieces);
}


static qu_ast_node *qu_raw_process_value(struct qu_parser *ctx,
    qu_ast_node *node, unsigned flags)
{
    const char *tag = node->tag;
    int i;
    for (i = 0; i < qu_tag_registry_len; ++i) {
        struct qu_tag_entry *e = &qu_tag_registry[i];
        if(e->flag & flags && !strcmp(e->tag, tag)) {
           return e->processor(ctx, node);
        }
    }
    return node;
}

static void qu_raw_visitor(struct qu_parser *ctx, qu_ast_node *node,
    struct qu_var_frame *vars, unsigned flags)
{
    switch(node->kind) {
    case QU_NODE_MAPPING: {
        qu_map_member *item;
        qu_map_index *map = &node->val.map_index;
        for(item = TAILQ_FIRST(&map->items); item;) {
            if(item->value->tag) {
                item->value = qu_raw_process_value(ctx, item->value, flags);
            }
            if(flags & QU_RAW_FLAG_TEMPLATE && item->value->tag &&
                !strncmp(item->value->tag, "!Template:", 10)) {
                qu_ast_node *nnode = qu_raw_template(ctx, item->value);
                qu_raw_visitor(ctx, nnode,
                    qu_node_frame(&ctx->pieces, item->value, vars),
                    flags);
                item->value = nnode;
            } else if(flags & QU_RAW_FLAG_NOVARS && item->value->tag &&
                !strcmp(item->value->tag, "!NoVars")) {
                item->value->tag = NULL;
                qu_raw_visitor(ctx, item->value,
                    NULL, flags & ~QU_RAW_FLAG_VARS);
            } else {
                qu_raw_visitor(ctx, item->value, vars, flags);
            }
            item = TAILQ_NEXT(item, lst);
        }
        } break;
    case QU_NODE_SEQUENCE: {
        qu_seq_index *seq = &node->val.seq_index;
        qu_seq_member *item;
        for(item = TAILQ_FIRST(&seq->items); item;) {
            if(item->value->tag) {
                item->value = qu_raw_process_value(ctx, item->value, flags);
            }
            if(flags & QU_RAW_FLAG_TEMPLATE && item->value->tag &&
                !strncmp(item->value->tag, "!Template:", 10)) {
                qu_ast_node *nnode = qu_raw_template(ctx, item->value);
                qu_raw_visitor(ctx, nnode,
                    qu_node_frame(&ctx->pieces, item->value, vars),
                    flags);
                item->value = nnode;
            } else if(flags & QU_RAW_FLAG_NOVARS && item->value->tag &&
                !strcmp(item->value->tag, "!NoVars")) {
                item->value->tag = NULL;
                qu_raw_visitor(ctx, item->value,
                    NULL, flags & ~QU_RAW_FLAG_VARS);
            } else {
                qu_raw_visitor(ctx, item->value, vars, flags);
            }
            item = TAILQ_NEXT(item, lst);
        }
        } break;
    case QU_NODE_SCALAR: {
        const char *content = qu_parse_content(node, &ctx->pieces);
        if(content) {
            if(flags & QU_RAW_FLAG_VARS) {
                qu_eval_str(ctx, vars, content, node,
                    &node->content, &node->content_len);
            } else {
                node->content = content;
                node->content_len = strlen(content);
            }
        } else {
            node->content = "";
            node->content_len = 0;
        }
        } break;
    default:
        break;
    }
}

void qu_raw_process(struct qu_parser *ctx, struct qu_var_frame *vars,
    unsigned flags)
{
    qu_raw_visitor(ctx, ctx->document, vars, flags);
    qu_raw_maps_visitor(ctx, ctx->document, flags);
}

unsigned qu_raw_flags_from_str(char *flags) {
    unsigned result = QU_RAW_FLAGS_NONE;
    if(strchr(flags, '*'))
        return ~0;
    if(strchr(flags, 'm'))
        result |= QU_RAW_FLAG_MERGE;
    if(strchr(flags, 'u'))
        result |= QU_RAW_FLAG_UNPACK;
    if(strchr(flags, 'a'))
        result |= QU_RAW_FLAG_ALIAS;
    if(strchr(flags, 'i'))
        result |= QU_RAW_FLAG_INCLUDE;
    if(strchr(flags, 'v'))
        result |= QU_RAW_FLAG_VARS;
    if(strchr(flags, 'V'))
        result |= QU_RAW_FLAG_NOVARS;
    if(strchr(flags, 't'))
        result |= QU_RAW_FLAG_TEMPLATE;
    if(*flags == '^')
        return ~result;
    return result;
}
