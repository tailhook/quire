#include <assert.h>

#include "optparser.h"
#include "context.h"
#include "../quire_int.h"

void qu_optparser_init(struct qu_config_context *ctx,
    int argc, char **argv,
    const struct qu_short_option *shopt,
    const struct qu_long_option *lopt)
{
    struct qu_optparser_struct *self = &ctx->optparser;
    self->argc = argc;
    self->argv = argv;
    self->shopt = shopt;
    self->lopt = lopt;

    self->argleft = argc - 1;
    self->last = argv;
    self->curshort = NULL;
    self->curarg = NULL;
    self->cur = NULL;
}

static void qu_optparser_short(struct qu_config_context *ctx,
    int *opt, const char **arg)
{
    const struct qu_short_option *shopt;
    struct qu_optparser_struct *self = &ctx->optparser;

    for(shopt = self->shopt; shopt->opt; ++shopt) {
        if(shopt->opt != *self->curshort)
            continue;
        if(shopt->has_arg) {
            if(*(self->curshort + 1)) {
                /*  Option and it's value in same argument  */
                *arg = self->curarg = self->curshort + 1;
            } else {
                if(!self->argleft)
                    qu_optparser_error(ctx, "Option requires an argument");
                self->last += 1;
                self->argleft -= 1;
                *arg = self->curarg = *self->last;
            }
        } else {
            *arg = NULL;
        }
        *opt = shopt->name;
        return;
    }
    qu_optparser_error(ctx, "Wrong short option");
}

static void qu_optparser_long(struct qu_config_context *ctx,
    int *opt, const char **arg)
{
    const struct qu_long_option *lopt;
    struct qu_optparser_struct *self = &ctx->optparser;
    const char *copt = self->cur;
    const char *coptend = strchr(self->cur, '=');
    int coptlen;

    if(coptend) {
        coptlen = coptend - copt;
    } else {
        coptlen = strlen(copt);
    }

    for(lopt = self->lopt; lopt->opt; ++lopt) {
        if(strncmp(lopt->opt, copt, coptlen))
            continue;
        if(coptlen != strlen(lopt->opt)) {
            if((lopt+1)->opt && !strncmp((lopt+1)->opt, copt, coptlen))
                qu_optparser_error(ctx, "Ambiguous option abbreviation");
        }

        if(lopt->has_arg) {
            if(coptend) {
                *arg = self->curarg = coptend + 1;
            } else {
                if(!self->argleft)
                    qu_optparser_error(ctx, "Option requires an argument");
                self->last += 1;
                self->argleft -= 1;
                *arg = self->curarg = *self->last;
            }
        } else {
            *arg = NULL;
        }
        *opt = lopt->name;
        return;
    }
    qu_optparser_error(ctx, "Wrong long option");
}

int qu_optparser_next(struct qu_config_context *ctx,
    int *opt, const char **arg)
{
    struct qu_optparser_struct *self = &ctx->optparser;
    if(self->curshort) {
        if(self->curarg) {
            /*  Last argument was short option with argument  */
        } else {
            /*  Last argument was short option without an argument  */
            self->curshort += 1;
            if(*self->curshort) {
                qu_optparser_short(ctx, opt, arg);
                return 1;
            } else {
                self->curshort = NULL;
            }
        }
    }
    if(!self->argleft)
        return 0;
    self->last += 1;
    self->argleft -= 1;
    self->cur = *self->last;
    assert(self->cur);
    self->curshort = NULL;
    if(self->cur[0] != '-' || self->cur[1] == 0 ||
        (self->cur[1] == '-' && self->cur[2] == 0))
        return 0;
    if(self->cur[1] == '-') {
        qu_optparser_long(ctx, opt, arg);
    } else {
        self->curshort = self->cur + 1;
        qu_optparser_short(ctx, opt, arg);
    }
    return 1;
}

void qu_optparser_error(struct qu_config_context *ctx,
    const char *error)
{
    struct qu_optparser_struct *self = &ctx->optparser;
    fprintf(stderr, "Error option ``%s''\n",
        self->curshort ? self->curshort : self->cur);
    LONGJUMP_WITH_CONTENT_ERROR(&ctx->parser, NULL, error);
}
