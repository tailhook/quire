#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <assert.h>


char *opt_string = "hf:H:C:e:";
struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"source", 0, NULL, 'f'},
    {"c-header", 0, NULL, 'H'},
    {"c-source", 0, NULL, 'C'},
    {"enable", 0, NULL, 'e'},
    {NULL, 0, NULL, 0}};

struct options {
    char *source_file;
    char *output_header;
    char *output_source;
    char *sections[128];
} options = {NULL, NULL, NULL, {NULL}};


void print_usage(FILE *out) {
    fprintf(out, "Usage:\n"
        "    cgen --source config.yaml [--c-header config.h]"
                                     " [--c-source config.c]\n"
        "\n"
        "Options:\n"
        " --source FILE    File name of the source yaml file\n"
        " --c-header FILE  Write C header to FILE\n"
        " --c-source FILE  Write parser source in C to FILE\n"
        " --enable COND    Enable section in config marked !If:COND\n"
        );
};

void parse_options(int argc, char **argv) {
    int c;
    int cursect = 0;
    while((c = getopt_long(argc, argv, opt_string, long_options, NULL)) != -1){
        switch(c) {
        case 'h':
            print_usage(stdout);
            exit(0);
        case 'f':
            options.source_file = optarg;
            break;
        case 'H':
            options.output_header = optarg;
            break;
        case 'C':
            options.output_source = optarg;
            break;
        case 'e':
            if(cursect > sizeof(options.sections)/sizeof(char *)-1) {
                fprintf(stderr, "Too many --enable flags\n");
                exit(7);
            }
            options.sections[cursect++] = optarg;
            break;
        default:
            print_usage(stderr);
            exit(1);
        }
    }
    if(optind) {
        fprintf(stderr, "cgen: unrecognized option '%s'\n", argv[optind]);
    }
    options.sections[cursect] = NULL;
}

int main(int argc, char **argv) {
    parse_options(argc, argv);
}
