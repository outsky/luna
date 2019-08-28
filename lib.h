#ifndef lib_h
#define lib_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


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

#endif
