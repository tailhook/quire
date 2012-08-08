#ifndef _H_OPTIONS
#define _H_OPTIONS

typedef struct qu_options_s {
    char *source_file;
    char *output_header;
    char *output_source;
    char *version_info;
    char *prefix;
    char *sections[128];
} qu_options_t;

void quire_parse_options(qu_options_t *opt, int argc, char **argv);

#endif // _H_OPTIONS
