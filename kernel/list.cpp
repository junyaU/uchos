#include "list.hpp"

void list_init(list_t* list)
{
	list->prev = list;
	list->next = list;
}

void list_elem_init(list_elem_t* elem)
{
	elem->prev = nullptr;
	elem->next = nullptr;
}

void list_push_back(list_t* list, list_elem_t* elem)
{
	list_elem_t* end = list->prev;
	end->next = elem;
	elem->prev = end;
	elem->next = list;
	list->prev = elem;
}

list_elem_t* list_pop_front(list_t* list)
{
	list_elem_t* front = list->next;
	if (front == list) {
		return nullptr;
	}

	list_elem_t* next = front->next;
	list->next = next;
	next->prev = list;

	return front;
}

bool list_contains(list_t* list, list_elem_t* elem)
{
	list_elem_t* node = list->next;
	while (node != list) {
		if (node == elem) {
			return true;
		}

		node = node->next;
	}

	return false;
}

bool list_is_empty(list_t* list) { return list->next == list; }