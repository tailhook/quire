#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "yparser.h"
#include "codes.h"


static qu_ast_node *process(qu_parse_context *ctx,
						   qu_ast_node *node, int flags)
{
	int tlen = node->tag->bytelen;
	char *tag = (char *)node->tag->data;
	if(tlen >= 10 && !strncmp(tag, "!FromFile:", 10)) {
		int rc, eno;
		unsigned char *data = NULL;
		int fd = open(tag + 10, O_RDONLY);
		if(fd < 0) {
			ctx->error_kind = YAML_SYSTEM_ERROR;
			ctx->error_text = "Can't open file";
			ctx->error_token = node->start_token;
			longjmp(ctx->errjmp, -errno);
		}
		struct stat stinfo;
		rc = fstat(fd, &stinfo);
		if(rc < 0) {
			eno = errno;
			close(fd);
			ctx->error_kind = YAML_SYSTEM_ERROR;
			ctx->error_text = "Can't stat file";
			ctx->error_token = node->start_token;
			longjmp(ctx->errjmp, -eno);
		}
		data = obstack_alloc(&ctx->pieces, stinfo.st_size+1);
		int so_far = 0;
		while(so_far < stinfo.st_size) {
			rc = read(fd, data + so_far, stinfo.st_size - so_far);
			if(rc == -1) {
				eno = errno;
				if(eno == EINTR) continue;
				close(fd);
				ctx->error_kind = YAML_SYSTEM_ERROR;
				ctx->error_text = "Can't read file";
				ctx->error_token = node->start_token;
				longjmp(ctx->errjmp, -eno);
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
		return node;
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
