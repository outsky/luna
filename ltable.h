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
void ltable_settable(ltable *lt, const char *key, const Value *v);
Value* ltable_gettable(ltable *lt, const char *key);
int ltable_len(const ltable *lt);

#endif
