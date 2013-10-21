#include <stdlib.h>
#include <string.h>


static struct unit_s {
    const char *unit;
    size_t value;
} units[] = {
    {"k", 1000L},
    {"ki", 1L << 10},
    {"M", 1000000L},
    {"Mi", 1L << 20},
    {"G", 1000000000L},
    {"Gi", 1L << 30},
    {"T", 1000000000000L},
    {"Ti", 1L << 40},
    {"P", 1000000000000000L},
    {"Pi", 1L << 50},
    {"E", 1000000000000000000L},
    {"Ei", 1L << 60},
    {NULL, 0},
    };

const char *bool_true[] = {
    "yes",
    "true",
    "y",
    NULL};

const char *bool_false[] = {
    "no",
    "false",
    "n",
    "~",
    "",
    NULL};


const char *qu_parse_int(const char *value, long *result) {
    const char *end;
    long val = strtol(value, (char **)&end, 0);
    if(*end) {
        for(struct unit_s *unit = units; unit->unit; ++unit) {
            if(!strcmp(end, unit->unit)) {
                val *= unit->value;
                end += strlen(unit->unit);
                break;
            }
        }
    }
    *result = val;
    return end;
}

int qu_parse_bool(const char *value, int *result) {
    const char **iter;
    for(iter = bool_true; *iter; ++iter) {
        if(!strcasecmp(value, *iter)) {
            *result = 1;
            return 1;
        }
    }
    for(iter = bool_false; *iter; ++iter) {
        if(!strcasecmp(value, *iter)) {
            *result = 0;
            return 1;
        }
    }
    return 0;
}
