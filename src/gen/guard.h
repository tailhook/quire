#ifndef QUIRE_H_GEN_GUARD
#define QUIRE_H_GEN_GUARD

struct qu_context;

struct qu_guard {
    const char *condition;
};

struct qu_guard *qu_guard_new(struct qu_context *ctx, const char *name);
void qu_guard_print_open(struct qu_context *ctx, struct qu_guard *guard);
void qu_guard_print_close(struct qu_context *ctx, struct qu_guard *guard);

#endif  /*  QUIRE_H_GEN_GUARD  */
