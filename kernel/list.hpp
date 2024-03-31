#pragma once

#include <cstddef>
struct list {
	list* prev;
	list* next;
};

using list_t = list;
using list_elem_t = list;

void list_init(list_t* list);
void list_elem_init(list_elem_t* elem);
void list_push_back(list_t* list, list_elem_t* elem);
list_elem_t* list_pop_front(list_t* list);
bool list_is_empty(list_t* list);
bool list_contains(list_t* list, list_elem_t* elem);
size_t list_size(list_t* list);

#define LIST_POP_FRONT(list, container, field)                                      \
	({                                                                              \
		list_elem_t* __elem = list_pop_front(list);                                 \
		(__elem) ? LIST_CONTAINER(__elem, container, field) : nullptr;              \
	})

#define LIST_CONTAINER(elem, container, field)                                      \
	((container*)((uintptr_t)(elem) - __builtin_offsetof(container, field)))