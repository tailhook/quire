#ifndef QUIRE_H_CFG_OPTPARSER
#define QUIRE_H_CFG_OPTPARSER

struct qu_config_context;

struct qu_short_option {
    char opt;
    int has_arg;
    int name;
};

struct qu_long_option {
    const char *opt;
    int has_arg;
    int name;
};

struct qu_optparser_struct {
    int argc;
    char **argv;
    const struct qu_short_option *shopt;
    const struct qu_long_option *lopt;

    int argleft;
    char **last;
    const char *cur;
    const char *curshort;
    const char *curarg;
};

void qu_optparser_init(struct qu_config_context *ctx,
    int argc, char **argv,
    const struct qu_short_option *shopt,
    const struct qu_long_option *lopt);
int qu_optparser_next(struct qu_config_context *ctx,
    int *opt, const char **arg);
void qu_optparser_error(struct qu_config_context *ctx,
    const char *error);

#endif  /* QUIRE_H_CFG_OPTPARSER */
