#ifndef QUIRE_H_GEN_TYPES
#define QUIRE_H_GEN_TYPES

struct qu_context;

struct qu_option_vptr {
};

struct qu_option {
    struct qu_option_vptr *vp;
};

struct qu_option *qu_option_new(struct qu_context *ctx,
    struct qu_option_vptr *vp);

#endif  // QUIRE_H_GEN_TYPES
