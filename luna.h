#ifndef luna_h
#define luna_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include "luna.h"


#define CAST(T, v) ((T)(v))
#define NEW_ARRAY(T, n) CAST(T*, calloc(n, sizeof(T)))
#define NEW(T) NEW_ARRAY(T, 1)
#define NEW_SIZE(T, size) CAST(T*, calloc(size, 1))
#define FREE(p) free(p); (p) = NULL

#define STR2(x) #x
#define STR(x) STR2(x)
#define error(...) errorf(__FILE__ ":" STR(__LINE__), __VA_ARGS__)

void errorf(const char *where, const char *fmt, ...);
void snapshot(const char* code, int pos, int line);
char* load_file(const char *filename);

struct ltable;
typedef enum {
    VT_INVALID,
    VT_INT,
    VT_FLOAT,
    VT_STRING,
    VT_BOOL,
    VT_NIL,
    VT_TABLE,
} ValueType;

typedef struct {
    ValueType t;
    union {
        int n;
        double f;
        char *s;
        struct ltable *lt;
    } u;
} Value;

void _copy_value(Value *dest, const Value *src);

#endif