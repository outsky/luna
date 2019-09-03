#ifndef list_h
#define list_h

typedef struct _lnode {
    void *data;
    struct _lnode *next;
} lnode;

typedef struct _list {
    lnode *head;
    lnode *tail;
    int count;
} list;

list* list_new();
void list_free(list *l);
int list_pushfront(list *l, void *data);
int list_pushback(list *l, void *data);
void list_remove(list *l, void *nd);

#endif
