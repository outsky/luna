#ifndef luna_h
#define luna_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include "luna.h"

#define MAX_NAME_LEN 32

#define CAST(T, v) ((T)(v))
#define NEW_ARRAY(T, n) CAST(T*, calloc(n, sizeof(T)))
#define NEW(T) NEW_ARRAY(T, 1)
#define NEW_SIZE(T, size) CAST(T*, calloc(size, 1))
#define FREE(p) free(p); (p) = NULL

#define STR2(x) #x
#define STR(x) STR2(x)
#define error(...) errorf(__FILE__ ":" STR(__LINE__), __VA_ARGS__)

#define FREAD(p, s, c, f) do {\
    if (c != fread(p, s, c, f)) {\
        error("fread failed1: %s", strerror(errno));\
    }\
} while (0)

void errorf(const char *where, const char *fmt, ...);
void snapshot(const char* code, int pos, int line);
char* load_file(const char *filename);

typedef enum {
    VT_NIL,
    VT_INT,
    VT_FLOAT,
    VT_STRING,
    VT_BOOL,
    VT_TABLE,
    VT_CLOSURE,
    VT_VALUEP,  /* pointer to Value */
    VT_CALLINFO,
} ValueType;

typedef struct {
    ValueType t;
    union {
        int n;
        double f;
        char *s;
        void *o;
    } u;
} Value;

void copy_value(Value *dest, const Value *src);

#endif
