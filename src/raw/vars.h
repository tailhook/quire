#ifndef QUIRE_H_RAW_VARS
#define QUIRE_H_RAW_VARS

struct obstack;
struct qu_parser;
struct qu_var_frame;
struct qu_variable;

void qu_var_set_string(struct obstack *buf, struct qu_var_frame *frame,
    const char *name, int nlen, const char *data, int dlen);
int qu_string_var(struct qu_var_frame *frame,
    const char *name, int name_len, const char **data, int *dlen);
int qu_anchor_var(struct qu_parser *ctx,
    const char *name, int name_len, const char **data, int *dlen);
struct qu_var_frame *qu_var_new_frame(
    struct obstack *buf, struct qu_var_frame *parent);

#endif  /* QUIRE_H_RAW_VARS */
