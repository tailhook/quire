#ifndef QUIRE_H_GEN_CODE
#define QUIRE_H_GEN_CODE

struct qu_context;
struct obstack;

void qu_code_print(struct qu_context *ctx, const char *template, ...);
const char *qu_template_alloc(struct qu_context *ctx,
    const char *template, ...);
void qu_template_grow(struct qu_context *ctx, const char *template, ...);
const char *qu_line_grow(struct obstack *buf,
    const char *ptr, const char *endptr, int width);

#endif  // QUIRE_H_GEN_CODE
