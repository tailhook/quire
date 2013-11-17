#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "error.h"

void qu_err_init(struct qu_errbuf *buf, jmp_buf *jmp) {
    buf->len = 0;
    buf->error = 0;
    buf->fatal = 0;
    buf->jmp = jmp;
}

static void qu_err_system_add(struct qu_errbuf *buf, int errnum,
    const char *fmt, va_list args) {
    int left = sizeof(buf->buf) - buf->len - 2;
    int added;
    added = vsnprintf(buf->buf + buf->len, left, fmt, args);
    if(left < added)
        added = left;
    if(added > 0) {
        left -= added;
        buf->len += added;
        added = snprintf(buf->buf + buf->len, left, ": %s", strerror(errnum));
        if(added < 0)
            added = 0;
        else if(added > left)
            added = left;
        buf->len += added + 1;
        buf->buf[buf->len-1] = '\n';
        buf->buf[buf->len] = 0;
    }
}

static void qu_err_file_add(struct qu_errbuf *buf, const char *fn, int line,
    const char *fmt, va_list args) {
    int left = sizeof(buf->buf) - buf->len - 2;
    int added;
    added = snprintf(buf->buf + buf->len, left,
        "Syntax error at \"%s\":%d: ", fn, line);
    if(left < added)
        added = left;
    if(added > 0) {
        left -= added;
        buf->len += added;
        added = vsnprintf(buf->buf + buf->len, left, fmt, args);
        if(added < 0)
            added = 0;
        else if(added > left)
            added = left;
        buf->len += added + 1;
        buf->buf[buf->len-1] = '\n';
        buf->buf[buf->len] = 0;
    }
}

static void qu_err_node_add(struct qu_errbuf *buf, qu_ast_node *node,
    const char *fmt, va_list args) {
    qu_token *tok = node->tag_token ? node->tag_token : node->start_token;
    qu_err_file_add(buf, tok->filename, tok->start_line, fmt, args);
}

static void qu_err_cli_add(struct qu_errbuf *buf, const char *opt,
    const char *fmt, va_list args) {
    int left = sizeof(buf->buf) - buf->len - 2;
    int added;
    if(opt[0] == '-') {  /*  Long option  */
        added = snprintf(buf->buf + buf->len, left, "Option \"%s\": ", opt);
    } else {
        added = snprintf(buf->buf + buf->len, left, "Option \"-%c\": ", *opt);
    }
    if(left < added)
        added = left;
    if(added > 0) {
        left -= added;
        buf->len += added;
        added = vsnprintf(buf->buf + buf->len, left, fmt, args);
        if(added < 0)
            added = 0;
        else if(added > left)
            added = left;
        buf->len += added + 1;
        buf->buf[buf->len-1] = '\n';
        buf->buf[buf->len] = 0;
    }
}

void qu_err_node_fatal(struct qu_errbuf *buf, qu_ast_node *node,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_node_add(buf, node, fmt, args);
    va_end(args);
    buf->error = 1;
    buf->fatal = 1;
    longjmp(*buf->jmp, 1);
}
void qu_err_node_warn(struct qu_errbuf *buf, qu_ast_node *node,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_node_add(buf, node, fmt, args);
    va_end(args);
}
void qu_err_node_error(struct qu_errbuf *buf, qu_ast_node *node,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_node_add(buf, node, fmt, args);
    va_end(args);
    buf->error = 1;
}

void qu_err_cli_fatal(struct qu_errbuf *buf, const char *opt,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_cli_add(buf, opt, fmt, args);
    va_end(args);
    buf->error = 1;
    buf->fatal = 1;
    longjmp(*buf->jmp, 1);
}
void qu_err_cli_warn(struct qu_errbuf *buf, const char *opt,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_cli_add(buf, opt, fmt, args);
    va_end(args);
}
void qu_err_cli_error(struct qu_errbuf *buf, const char *opt,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_cli_add(buf, opt, fmt, args);
    va_end(args);
    buf->error = 1;
}

void qu_err_system_fatal(struct qu_errbuf *buf, int errnum,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_system_add(buf, errnum, fmt, args);
    va_end(args);
    buf->error = 1;
    buf->fatal = 1;
    longjmp(*buf->jmp, 1);
}
void qu_err_system_warn(struct qu_errbuf *buf, int errnum,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_system_add(buf, errnum, fmt, args);
    va_end(args);
}
void qu_err_system_error(struct qu_errbuf *buf, int errnum,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_system_add(buf, errnum, fmt, args);
    va_end(args);
    buf->error = 1;
}

void qu_err_file_fatal(struct qu_errbuf *buf, const char *fn, int line,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_file_add(buf, fn, line, fmt, args);
    va_end(args);
    buf->error = 1;
    buf->fatal = 1;
    longjmp(*buf->jmp, 1);
}
void qu_err_file_warn(struct qu_errbuf *buf, const char *fn, int line,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_file_add(buf, fn, line, fmt, args);
    va_end(args);
}
void qu_err_file_error(struct qu_errbuf *buf, const char *fn, int line,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    qu_err_file_add(buf, fn, line, fmt, args);
    va_end(args);
    buf->error = 1;
}

void qu_print_errors(struct qu_errbuf *self, FILE *stream) {
    if(self->len) {
        fprintf(stream, "%s", self->buf);
    }
}
