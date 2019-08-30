#ifndef htable_h
#define htable_h

#include "list.h"

#define HT_MAX_KEY_LEN 32
typedef struct {
    char key[HT_MAX_KEY_LEN];
    void *value;
} hnode;

typedef struct {
    int size;
    list **slots;
} htable;

htable* htable_new(int size);
void htable_free(htable *h);
void htable_add(htable *h, const char *key, void *value);
void htable_remove(htable *h, const char *key);
void* htable_find(const htable *h, const char *key);

#endif
