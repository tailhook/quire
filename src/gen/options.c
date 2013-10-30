#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#include "options.h"


char *opt_string = "hf:H:C:e:p:";
struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"source", 1, NULL, 'f'},
    {"prefix", 1, NULL, 'p'},
    {"c-header", 1, NULL, 'H'},
    {"c-source", 1, NULL, 'C'},
    {"enable", 1, NULL, 'e'},
    {"version-info", 1, NULL, 'V'},
    {NULL, 0, NULL, 0}};

void quire_print_usage(FILE *out) {
    fprintf(out, "Usage:\n"
        "    cgen --source config.yaml [--c-header config.h]"
                                     " [--c-source config.c]\n"
        "\n"
        "Options:\n"
        " --source FILE    File name of the source yaml file\n"
        " --prefix NAME    Prefix for all declarations of the config\n"
        " --c-header FILE  Write C header to FILE\n"
        " --c-source FILE  Write parser source in C to FILE\n"
        " --enable COND    Enable section in config marked !If:COND\n"
        " --version-info DATA\n"
        "                  Version info to put into command-line help\n"
        );
};

void quire_parse_options(qu_options_t *opt, int argc, char **argv) {
    opt->prefix = "cfg";
    int c;
    int cursect = 0;
    while((c = getopt_long(argc, argv, opt_string, long_options, NULL)) != -1){
        switch(c) {
        case 'h':
            quire_print_usage(stdout);
            exit(0);
        case 'f':
            opt->source_file = optarg;
            break;
        case 'H':
            opt->output_header = optarg;
            break;
        case 'C':
            opt->output_source = optarg;
            break;
        case 'p':
            opt->prefix = optarg;
            break;
        case 'V':
            opt->version_info = optarg;
            break;
        case 'e':
            if(cursect > (int)(sizeof(opt->sections)/sizeof(char *)-1)) {
                fprintf(stderr, "Too many --enable flags\n");
                exit(7);
            }
            opt->sections[cursect++] = optarg;
            break;
        default:
            quire_print_usage(stderr);
            exit(1);
        }
    }
    if(optind < argc) {
        fprintf(stderr, "cgen: unrecognized option '%s'\n", argv[optind]);
        exit(1);
    }
    opt->sections[cursect] = NULL;
}
