#include <stdio.h>
#include <string.h>

#include "comprehensive-config.h"

cfg_main_t config;

/*
int convert_connectaddr(coyaml_parseinfo_t *info, char *value,
    coyaml_group_t * group, cfg_connectaddr_t * target) {
    if(!value || !*value) {
        fprintf(stderr, "Error parsing address ``%s''", value);
        return -1;
    }
    if(value[0] == '/' || (value[0] == '.' && value[1] == '/')) { // Unix sock
        if(info) {
            target->unix_socket = obstack_copy0(&info->head->pieces,
                value, strlen(value));
            target->unix_socket_len = strlen(value);
        } else {
            target->unix_socket = value;
            target->unix_socket_len = strlen(value);
        }
    } else { // TCP socket
        char *pos = strchr(value, ':');
        if(pos) { // Has port
            if(info) {
                target->host = obstack_copy0(&info->head->pieces,
                    value, pos-value);
                target->host_len = pos-value;
            } else {
                target->host = value;
                target->host_len = pos-value;
                *pos = 0;
            }
            target->port = atoi(pos+1);
        } else { // Has no port
            if(info) {
                target->host = obstack_copy0(&info->head->pieces,
                    value, strlen(value));
                target->host_len = strlen(value);
            } else {
                target->host = value;
                target->host_len = strlen(value);
            }
        }
    }
    return 0;
}

int convert_listenaddr(coyaml_parseinfo_t *info, char *value,
    coyaml_group_t * group, cfg_listenaddr_t * target) {
    if(!value || !*value) {
        fprintf(stderr, "Error parsing address ``%s''", value);
        return -1;
    }
    if(value[0] == '/' || (value[0] == '.' && value[1] == '/')) { // Unix sock
        if(info) {
            target->unix_socket = obstack_copy0(&info->head->pieces,
                value, strlen(value));
        } else {
            target->unix_socket = value;
        }
    } else if(value[0] == '&') { // file descriptor
        target->fd = atoi(value+1);
    } else { // TCP socket
        char *pos = strchr(value, ':');
        if(pos) { // Has port
            if(info) {
                target->host = obstack_copy0(&info->head->pieces,
                    value, pos-value);
            } else {
                target->host = value;
                *pos = 0;
            }
            target->port = atoi(pos+1);
        } else { // Has no port
            if(info) {
                target->host = obstack_copy0(&info->head->pieces,
                    value, strlen(value));
            } else {
                target->host = value;
            }
        }
    }
    return 0;
}
*/

int main(int argc, char **argv) {
    /*
    coyaml_context_t *ctx = cfg_context(NULL, &config);
    if(!ctx) {
        perror(argv[0]);
        return 1;
    }
    coyaml_cli_prepare_or_exit(ctx, argc, argv);
    coyaml_set_string(ctx, "hello", "example", strlen("example"));
    coyaml_set_integer(ctx, "intvar", 123);
    coyaml_readfile_or_exit(ctx);
    coyaml_env_parse_or_exit(ctx);
    coyaml_cli_parse_or_exit(ctx, argc, argv);
    coyaml_context_free(ctx);
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
