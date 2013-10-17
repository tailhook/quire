#include <obstack.h>
#include <ctype.h>

#include "wrap.h"

const char *qu_line_grow(struct obstack *buf,
    const char *ptr, const char *endptr, int width)
{
    const char *wordend;
    int left = width;
    while(1) {
        while(isspace(*ptr) && ptr < endptr) ++ptr;
        wordend = ptr;
        if(*ptr == '[') {
            while(*wordend != ']' && wordend < endptr) ++wordend;
            if(wordend < endptr)
                ++wordend;
        } else {
            while(!isspace(*wordend) && wordend < endptr) ++wordend;
        }
        int wlen = wordend - ptr;
        if(left == width) {
            obstack_grow(buf, ptr, wlen);
            left -= wlen;
        } else if(wlen+1 <= left) {
            obstack_1grow(buf, ' ');
            left -= 1;
            obstack_grow(buf, ptr, wlen);
            left -= wlen;
        } else {
            return ptr;
        }
        ptr = wordend;
        if(left <= 0 || ptr == endptr)
            return ptr;
    }
}

const char *qu_line_print(FILE *out,
    const char *ptr, const char *endptr, int width)
{
    const char *wordend;
    int left = width;
    while(1) {
        while(isspace(*ptr) && ptr < endptr) ++ptr;
        wordend = ptr;
        if(*ptr == '[') {
            while(*wordend != ']' && wordend < endptr) ++wordend;
            if(wordend < endptr)
                ++wordend;
        } else {
            while(!isspace(*wordend) && wordend < endptr) ++wordend;
        }
        int wlen = wordend - ptr;
        if(left == width) {
            fwrite(ptr, 1, wlen, out);
            left -= wlen;
        } else if(wlen+1 <= left) {
            putc(' ', out);
            left -= 1;
            fwrite(ptr, 1, wlen, out);
            left -= wlen;
        } else {
            return ptr;
        }
        ptr = wordend;
        if(left <= 0 || ptr == endptr)
            return ptr;
    }
}
