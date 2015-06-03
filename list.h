#ifndef _TIMING_WHEEL_LIST_H_
#define _TIMING_WHEEL_LIST_H_

#define offset_of(type, member) (size_t)&(((type*)0)->member)

#define container_of(memptr, type, member) \
({ \
    const typeof(((type*)0)->member)* _memptr = (memptr); \
    (type*)((char*)_memptr - offset_of(type, member)); \
})

typedef struct list_node
{
    struct list_node* prev;
    struct list_node* next;
}list_node_t;

void _list_add(list_node_t* node, list_node_t* prev, list_node_t* next);

void list_init(list_node_t* head);

void list_add(list_node_t* head, list_node_t* node);

void list_del(list_node_t* node);

int list_empty(list_node_t* head);

#endif
