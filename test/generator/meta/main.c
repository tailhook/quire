#include "config.h"

int main(int argc, char **argv) {
    struct cfg_main cfg;
    cfg_load(&cfg, argc, argv);
    cfg_free(&cfg);
    return 0;
}
