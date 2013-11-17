#ifndef QUIRE_H_ERROR
#define QUIRE_H_ERROR

#include <stdio.h>
#include <setjmp.h>

#include "hints.h"
#include "yaml/node.h"

struct qu_errbuf {
    char buf[2048];
    int len;
    int error:1;
    int fatal:1;
    jmp_buf *jmp;
};

void qu_err_init(struct qu_errbuf *buf, jmp_buf *jmp);

void qu_err_node_fatal(struct qu_errbuf *buf, qu_ast_node *node,
    const char *fmt, ...) QU_A_FORMAT(3) QU_A_NO_RETURN;
void qu_err_node_warn(struct qu_errbuf *buf, qu_ast_node *node,
    const char *fmt, ...) QU_A_FORMAT(3);
void qu_err_node_error(struct qu_errbuf *buf, qu_ast_node *node,
    const char *fmt, ...) QU_A_FORMAT(3);

void qu_err_cli_warn(struct qu_errbuf *buf, const char *opt,
    const char *fmt, ...) QU_A_FORMAT(3);
void qu_err_cli_error(struct qu_errbuf *buf, const char *opt,
    const char *fmt, ...) QU_A_FORMAT(3);
void qu_err_cli_fatal(struct qu_errbuf *buf, const char *opt,
    const char *fmt, ...) QU_A_FORMAT(3) QU_A_NO_RETURN;

void qu_err_system_fatal(struct qu_errbuf *buf, int errnum,
    const char *fmt, ...) QU_A_FORMAT(3) QU_A_NO_RETURN;
void qu_err_system_warn(struct qu_errbuf *buf, int errnum,
    const char *fmt, ...) QU_A_FORMAT(3);
void qu_err_system_error(struct qu_errbuf *buf, int errnum,
    const char *fmt, ...) QU_A_FORMAT(3);

void qu_err_file_fatal(struct qu_errbuf *buf, const char *fn, int line,
    const char *fmt, ...) QU_A_FORMAT(4) QU_A_NO_RETURN;
void qu_err_file_warn(struct qu_errbuf *buf, const char *fn, int line,
    const char *fmt, ...) QU_A_FORMAT(4);
void qu_err_file_error(struct qu_errbuf *buf, const char *fn, int line,
    const char *fmt, ...) QU_A_FORMAT(4);

/*  Public API  */
void qu_print_errors(struct qu_errbuf *self, FILE *stream);

#endif  /* QUIRE_H_ERROR */
