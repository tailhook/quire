#include <stddef.h>
#include <coyaml_hdr.h>

#ifndef COYAML_PARSEINFO
typedef struct coyaml_parseinfo_s {
} coyaml_parseinfo_t;
#endif

typedef void (*coyaml_convert_fun)(coyaml_parseinfo_t *info,
    char *value, int value_len,
    void *prop, void *target);
typedef void (*coyaml_state_fun)(coyaml_parseinfo_t *info,
    void *prop, void *target);

typedef struct coyaml_transition_s {
    char *symbol;
    coyaml_state_fun callback;
    void *prop;
} coyaml_transition_t;

typedef struct coyaml_tag_s {
    char *tagname;
    int tagvalue;
} coyaml_tag_t;

typedef struct coyaml_group_s {
    int baseoffset;
    int bitmask;
    coyaml_transition_t *transitions;
} coyaml_group_t;

typedef struct coyaml_usertype_s {
    int baseoffset;
    int bitmask;
    coyaml_tag_t *tags;
    coyaml_convert_fun *scalar_fun;
} coyaml_usertype_t;

typedef struct coyaml_custom_s {
    int baseoffset;
    int bitmask;
    coyaml_transition_t *transitions;
} coyaml_custom_t;

typedef struct coyaml_int_s {
    int baseoffset;
    int bitmask;
    int min;
    int max;
} coyaml_int_t;

typedef struct coyaml_uint_s {
    int baseoffset;
    int bitmask;
    unsigned int min;
    unsigned int max;
} coyaml_uint_t;

typedef struct coyaml_bool_s {
    int baseoffset;
} coyaml_bool_t;

typedef struct coyaml_float_s {
    int baseoffset;
    int bitmask;
    double min;
    double max;
} coyaml_float_t;

typedef struct coyaml_array_s {
    int elementoffset;
    coyaml_state_fun element_callback;
} coyaml_array_t;

typedef struct coyaml_mapping_s {
    int keyoffset;
    coyaml_state_fun key_callback;
    int valueoffset;
    coyaml_state_fun value_callback;
} coyaml_mapping_t;

typedef struct coyaml_file_s {
    int baseoffset;
    int bitmask;
    bool check_existence;
    bool check_dir;
    bool check_writable;
    char *warn_outside;
} coyaml_file_t;

typedef struct coyaml_dir_s {
    int baseoffset;
    int bitmask;
    bool check_existence;
    bool check_dir;
} coyaml_dir_t;

typedef struct coyaml_string_s {
    int baseoffset;
} coyaml_string_t;

void coyaml_CGroup(coyaml_parseinfo_t *info, coyaml_group_t *prop, void *target);
void coyaml_CInt(coyaml_parseinfo_t *info, coyaml_int_t *prop, void *target);
void coyaml_CUInt(coyaml_parseinfo_t *info, coyaml_uint_t *prop, void *target);
void coyaml_Bool(coyaml_parseinfo_t *info, coyaml_bool_t *prop, void *target);
void coyaml_CFloat(coyaml_parseinfo_t *info, coyaml_float_t *prop, void *target);
void coyaml_CArray(coyaml_parseinfo_t *info, coyaml_array_t *prop, void *target);
void coyaml_CMapping(coyaml_parseinfo_t *info, coyaml_mapping_t *prop, void *target);
void coyaml_CFile(coyaml_parseinfo_t *info, coyaml_file_t *prop, void *target);
void coyaml_CDir(coyaml_parseinfo_t *info, coyaml_dir_t *prop, void *target);
void coyaml_CString(coyaml_parseinfo_t *info, coyaml_string_t *prop, void *target);
void coyaml_CCustom(coyaml_parseinfo_t *info, coyaml_custom_t *prop, void *target);

int coyaml_readfile(char *filename, coyaml_group_t *root,
    void *target, bool debug);
