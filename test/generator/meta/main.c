#include "config.h"

int main(int argc, char **argv) {
    cfg_main_t cfg;
    cfg_load(&cfg, argc, argv);
    cfg_free(&cfg);
    return 0;
}
