#include "ltable.h"

ltable* ltable_new(int arraysize) {
    ltable *t = NEW(ltable);
    t->arraysize = arraysize;
    t->array = NEW_ARRAY(Value, arraysize);
    t->hash = htable_new(1024); /* TODO: hard coded */
    t->arraysize = arraysize;
    return t;
}

void ltable_setarray(ltable *lt, int idx, const Value *v) {
    if (idx >= lt->arraysize) {
        error("array index overflow: index %d size %d", idx, lt->arraysize);
    }
    _copy_value(&lt->array[idx], v);
}