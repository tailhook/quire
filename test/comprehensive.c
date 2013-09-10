#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comprehensive-config.h"

cfg_main_t config;

int convert_connectaddr(qu_parse_context *ctx, cfg_main_t *cfg,
	cfg_connectaddr_t * target, char *value)
{
    if(!value || !*value) {
        fprintf(stderr, "Error parsing address ``%s''", value);
        return -1;
    }
    if(value[0] == '/' || (value[0] == '.' && value[1] == '/')) { // Unix sock
		int dlen = strlen(value);
		char *data = qu_config_alloc(cfg, dlen);
		memcpy(data, value, dlen);
		target->unix_socket = data;
		target->unix_socket_len = dlen;
    } else { // TCP socket
        char *pos = strchr(value, ':');
        if(pos) { // Has port
			char *data = qu_config_alloc(cfg, pos-value);
			memcpy(data, value, pos-value);
			target->host = data;
			target->host_len = pos-value;
            target->port = atoi(pos+1);
        } else { // Has no port
			int dlen = strlen(value);
			char *data = qu_config_alloc(cfg, dlen);
			memcpy(data, value, dlen);
			target->host = data;
			target->host_len = dlen;
        }
    }
    return 0;
}

int convert_listenaddr(qu_parse_context *ctx, cfg_main_t *cfg,
	cfg_listenaddr_t * target, char *value)
{
    if(!value || !*value) {
        fprintf(stderr, "Error parsing address ``%s''", value);
        return -1;
    }
    if(value[0] == '/' || (value[0] == '.' && value[1] == '/')) { // Unix sock
		int dlen = strlen(value);
		char *data = qu_config_alloc(cfg, dlen);
		memcpy(data, value, dlen);
		target->unix_socket = data;
		target->unix_socket_len = dlen;
    } else if(value[0] == '&') { // file descriptor
        target->fd = atoi(value+1);
    } else { // TCP socket
        char *pos = strchr(value, ':');
        if(pos) { // Has port
			char *data = qu_config_alloc(cfg, pos-value);
			memcpy(data, value, pos-value);
			target->host = data;
			target->host_len = pos-value;
            target->port = atoi(pos+1);
        } else { // Has no port
			int dlen = strlen(value);
			char *data = qu_config_alloc(cfg, dlen);
			memcpy(data, value, dlen);
			target->host = data;
			target->host_len = dlen;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    /*
    printf("TAG: %d\n", config.SimpleHTTPServer.intvalue.tag);
    CFG_STRING_LOOP(item, config.SimpleHTTPServer.directory_indexes) {
        printf("INDEX: \"%s\"\n", item->value);
    }
    CFG_STRING_STRING_LOOP(item, config.SimpleHTTPServer.extra_headers) {
        printf("HEADER: \"%s\": \"%s\"\n", item->key, item->value);
    }*/
	int rc;
	cfg_main_t ccfg;
	cfg_main_t *cfg = &ccfg;
    qu_config_init(cfg, sizeof(*cfg));
    cfg_set_defaults(cfg);

    // Prepare context
    qu_parse_context cctx;
    qu_parse_context *ctx = &cctx;
    qu_parser_init(ctx);
	qu_set_string(ctx, "hello", "example");
	qu_set_integer(ctx, "intvar", 123);

    // Parsing command-line options
    cfg_cli_t ccli;
    cfg_cli_t *cli = &ccli;
    memset(cli, 0, sizeof(cfg_cli_t));
    cfg_cli_parse(ctx, cli, argc, argv);
    cfg_do_parse(ctx, cli, cfg);
    // Overlay command-line options on top
    rc = cfg_cli_apply(cfg, cli);
    if(rc < 0) {
        perror("simplehttp: libquire: Error applying command-line args");
        exit(127);
    }

    // Free resources
    qu_parser_free(ctx);
    cfg_print(cfg, 0, stdout);
    return 0;
}
