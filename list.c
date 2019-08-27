#include <stdlib.h>
#include "list.h"

static lnode* _newnode(void *data) {
    lnode *n = (lnode*)malloc(sizeof(*n));
    n->data = data;
    n->next = NULL;
    return n;
}

list* list_new() {
    list *l = (list*)malloc(sizeof(*l));
    l->head = l->tail = NULL;
    l->count = 0;
    return l;
}

void list_free(list *l) {
    if (l == NULL) {
        return;
    }

    lnode *n = l->head;
    for (;;) {
        if (n == NULL) {
            break;
        }

        lnode *tmp = n;
        n = n->next;

        free(tmp->data);
        free(tmp);
    }
    free(l);
}

int list_pushback(list *l, void *data) {
    lnode *n = _newnode(data);
    if (l->count == 0) {
        l->head = l->tail = n;
    } else {
        l->tail->next = n;
        l->tail = n;
    }
    return l->count++;
}
