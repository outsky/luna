#include "ltable.h"

ltable* ltable_new(int arraysize) {
    ltable *t = NEW(ltable);
    t->arraysize = arraysize;
    t->array = NEW_ARRAY(Value, arraysize);
    t->hash = htable_new(1024); /* TODO: hard coded */
    t->arraysize = arraysize;
    return t;
}

void ltable_free(ltable *lt) {
}

void ltable_setarray(ltable *lt, int idx, const Value *v) {
    if (idx >= lt->arraysize) {
        error("array index overflow: index %d size %d", idx, lt->arraysize);
    }
    copy_value(&lt->array[idx], v);
}

void ltable_settable(ltable *lt, const char *key, const Value *v) {
    Value *vcopy = NEW(Value);
    copy_value(vcopy, v);
    htable_add(lt->hash, key, vcopy);
}

Value* ltable_gettable(ltable *lt, const char *key) {
    return CAST(Value*, htable_find(lt->hash, key));
}

int ltable_len(const ltable *lt) {
    return lt->arraysize;
}
