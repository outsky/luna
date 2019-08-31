#include "luna.h"
#include "htable.h"

htable* htable_new(int size) {
    htable *h = NEW(htable);
    h->size = size;
    h->slots = NEW_ARRAY(list*, size);
    for (int i = 0; i < size; ++i) {
        h->slots[i] = list_new();
    }
    return h;
}

void htable_free(htable *h) {
    for (int i = 0; i < h->size; ++i) {
        list_free(h->slots[i]);
    }
    FREE(h);
}

static unsigned int _hash(const char *key) {
    unsigned int h = 0;
    for (int i = 0; i < strlen(key); ++i) {
        h = 31 * h + CAST(unsigned int, key[i]);
    }
    return h;
}

void htable_add(htable *h, const char *key, void *value) {
    htable_remove(h, key);

    int sidx = _hash(key) % h->size;
    hnode *hn = NEW(hnode);
    strcpy(hn->key, key);
    hn->value = value;
    list_pushback(h->slots[sidx], hn);
}

void htable_remove(htable *h, const char *key) {
    int sidx = _hash(key) % h->size;
    list *l = h->slots[sidx];
    for (lnode *n = l->head; n != NULL; n = n->next) {
        hnode *hn = CAST(hnode*, n->data);
        if (strcmp(key, hn->key) == 0) {
            list_remove(l, hn);
            break;
        }
    }
}

void* htable_find(const htable *h, const char *key) {
    int sidx = _hash(key) % h->size;
    const list *l = h->slots[sidx];
    for (lnode *n = l->head; n != NULL; n = n->next) {
        hnode *hn = CAST(hnode*, n->data);
        if (strcmp(key, hn->key) == 0) {
            return hn->value;
        }
    }
    return NULL;
}
