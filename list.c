#include "list.h"

void _list_add(list_node_t* node, list_node_t* prev, list_node_t* next)
{
    next->prev = node;
    node->next = next;
    prev->next = node;
    node->prev = prev;
}

void list_init(list_node_t* head)
{
    head->prev = head;
    head->next = head;
}

void list_add(list_node_t* head, list_node_t* node)
{
    _list_add(node, head->prev, head);
}

void list_del(list_node_t* node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    list_init(node);
}

int list_empty(list_node_t* head)
{
    return head->next == head ? 1 : 0;
}


