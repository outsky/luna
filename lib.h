#ifndef lib_h
#define lib_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NEW(T) (T)calloc(1, sizeof(T))
#define NEW_SIZE(T, size) (T)calloc(size, 1)
#define FREE(p) free(p); (p) = NULL

#define STR2(x) #x
#define STR(x) STR2(x)
#define error(...) errorf(__FILE__ ":" STR(__LINE__), __VA_ARGS__)

void errorf(const char *where, const char *fmt, ...);
void snapshot(const char* code, int pos, int line);
char* load_file(const char *filename);

#endif
