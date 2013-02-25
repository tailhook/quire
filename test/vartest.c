#include <stdio.h>
#include <getopt.h>

#include "vartest-config.h"

cfg_main_t config;

int main(int argc, char **argv) {
    cfg_load(&config, argc, argv);
    for(int i = optind; i < argc; ++i) {
        printf("option: %s\n", argv[i]);
    }
    cfg_free(&config);
}
