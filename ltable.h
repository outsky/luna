#ifndef ltable_h
#define ltable_h

#include "luna.h"
#include "htable.h"

typedef struct ltable {
    int arraysize;
    Value *array;
    htable *hash;
} ltable;

ltable* ltable_new(int arraysize);
void ltable_setarray(ltable *lt, int idx, const Value *v);

#endif
