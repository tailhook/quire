#include "config.h"

int main(int argc, char **argv) {
    config_main_t cfg;
    config_load(&cfg, args, argv);
    config_free(&cfg);
    return 0;
}
