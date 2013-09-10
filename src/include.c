#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>

#include "yparser.h"
#include "codes.h"
#include "access.h"


static char *join_filenames(qu_parse_context *ctx, char *base, char *target) {
	if(!target)
		return NULL;
	if(*target == '/')
		return target;
	char *slash = strrchr(base, '/');
	if(!slash)
		return target;
	obstack_blank(&ctx->pieces, 0);
	obstack_grow(&ctx->pieces, base, slash - base + 1);
	obstack_grow0(&ctx->pieces, target, strlen(target));
	return obstack_finish(&ctx->pieces);
}


static qu_ast_node *process(qu_parse_context *ctx,
						   qu_ast_node *node, int flags)
{
	int tlen = node->tag->bytelen;
	char *tag = (char *)node->tag->data;
	if(tlen == 9 && !strncmp(tag, "!FromFile", 9)) {
		int rc, eno;
		unsigned char *data = NULL;
		int fd = open(join_filenames(ctx, node->tag->filename,
									 qu_node_content(node)), O_RDONLY);
		if(fd < 0) {
			LONGJUMP_WITH_SYSTEM_ERROR(ctx, node->start_token,
				"Can't open file");
		}
		struct stat stinfo;
		rc = fstat(fd, &stinfo);
		if(rc < 0) {
			eno = errno;
			close(fd);
			LONGJUMP_WITH_SYSTEM_ERROR(ctx, node->start_token,
				"Can't stat file");
		}
		data = obstack_alloc(&ctx->pieces, stinfo.st_size+1);
		int so_far = 0;
		while(so_far < stinfo.st_size) {
			rc = read(fd, data + so_far, stinfo.st_size - so_far);
			if(rc == -1) {
				eno = errno;
				if(eno == EINTR) continue;
				close(fd);
				LONGJUMP_WITH_SYSTEM_ERROR(ctx, node->start_token,
					"Error reading file");
			}
			if(!rc) {
				// WARNING: file truncated
				break;
			}
			so_far += rc;
		}
		close(fd);
		data[so_far] = 0;
		node->content = (char *)data;
		node->content_len = so_far;
		node->tag = NULL;
		return node;
	} else if(tlen == 8 && !strncmp(tag, "!Include", 8)) {
		qu_ast_node *newone = qu_file_newparse(ctx,
			join_filenames(ctx, node->tag->filename, qu_node_content(node)));
		assert(newone);  // usually longjumps on error
		return newone;
	}
	return node;
}

static void visitor(qu_parse_context *ctx, qu_ast_node *node, int flags) {
    switch(node->kind) {
    case QU_NODE_MAPPING: {
        qu_map_index *map = &node->val.map_index;
        for(qu_map_member *item = TAILQ_FIRST(&map->items); item;) {
            if(item->value->tag) {
				item->value = process(ctx, item->value, flags);
			}
			visitor(ctx, item->value, flags);
			item = TAILQ_NEXT(item, lst);
        }
        } break;
    case QU_NODE_SEQUENCE: {
        qu_seq_index *seq = &node->val.seq_index;
        for(qu_seq_member *item = TAILQ_FIRST(&seq->items); item;) {
            if(item->value->tag) {
				item->value = process(ctx, item->value, flags);
			}
			visitor(ctx, item->value, flags);
			item = TAILQ_NEXT(item, lst);
        }
        } break;
    }
}

void _qu_process_includes(qu_parse_context *ctx, int flags) {
    visitor(ctx, ctx->document, flags);
}
