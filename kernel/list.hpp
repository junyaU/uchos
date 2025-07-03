/**
 * @file list.hpp
 * @brief Intrusive doubly-linked list implementation
 * 
 * This file provides a lightweight intrusive linked list implementation
 * commonly used in kernel code. The list nodes are embedded directly
 * into the structures that need to be linked, avoiding separate allocations.
 * This design is efficient and cache-friendly for kernel data structures.
 * 
 * @date 2024
 */

#pragma once

#include <cstddef>
/**
 * @brief Intrusive doubly-linked list node structure
 * 
 * This structure should be embedded in any struct that needs to be
 * part of a linked list. A single struct can contain multiple list
 * nodes to be part of multiple lists simultaneously.
 */
struct list {
	list* prev;  ///< Pointer to previous node in the list
	list* next;  ///< Pointer to next node in the list
};

/**
 * @brief Type alias for list head
 * 
 * The list head is just a list node that serves as the anchor point
 * for the entire list. It is not part of any containing structure.
 */
using list_t = list;

/**
 * @brief Type alias for list element
 * 
 * List elements are list nodes embedded within other structures.
 */
using list_elem_t = list;

/**
 * @brief Initialize a list head
 * 
 * Sets up an empty list by making the head point to itself.
 * This must be called before using any other list operations.
 * 
 * @param list Pointer to the list head to initialize
 */
void list_init(list_t* list);

/**
 * @brief Initialize a list element
 * 
 * Prepares a list element for insertion by clearing its pointers.
 * This should be called on elements before adding them to a list.
 * 
 * @param elem Pointer to the list element to initialize
 */
void list_elem_init(list_elem_t* elem);

/**
 * @brief Add an element to the end of the list
 * 
 * Inserts the element at the tail of the list, making it the new last element.
 * 
 * @param list Pointer to the list head
 * @param elem Pointer to the element to add
 * 
 * @note The element must not already be in a list
 */
void list_push_back(list_t* list, list_elem_t* elem);

/**
 * @brief Remove and return the first element from the list
 * 
 * Removes the element at the head of the list and returns it.
 * 
 * @param list Pointer to the list head
 * @return Pointer to the removed element, or nullptr if list is empty
 */
list_elem_t* list_pop_front(list_t* list);

/**
 * @brief Check if the list is empty
 * 
 * @param list Pointer to the list head
 * @return true if the list contains no elements, false otherwise
 */
bool list_is_empty(list_t* list);

/**
 * @brief Check if an element is in the list
 * 
 * Searches the list to determine if the given element is present.
 * 
 * @param list Pointer to the list head
 * @param elem Pointer to the element to search for
 * @return true if the element is in the list, false otherwise
 * 
 * @note This is an O(n) operation
 */
bool list_contains(list_t* list, list_elem_t* elem);

/**
 * @brief Count the number of elements in the list
 * 
 * @param list Pointer to the list head
 * @return Number of elements in the list
 * 
 * @note This is an O(n) operation
 */
size_t list_size(list_t* list);

/**
 * @brief Pop element from front of list and convert to containing structure
 * 
 * This macro combines list_pop_front() with container conversion in a
 * single operation. It removes the first element and returns a pointer
 * to the containing structure.
 * 
 * @param list Pointer to the list head
 * @param container Type name of the containing structure
 * @param field Name of the list_elem_t field within the container
 * @return Pointer to the containing structure, or nullptr if list is empty
 * 
 * @example
 * struct task {
 *     int pid;
 *     list_elem_t list_node;
 * };
 * task* t = LIST_POP_FRONT(&task_list, task, list_node);
 */
#define LIST_POP_FRONT(list, container, field)                                      \
	({                                                                              \
		list_elem_t* __elem = list_pop_front(list);                                 \
		(__elem) ? LIST_CONTAINER(__elem, container, field) : nullptr;              \
	})

/**
 * @brief Convert list element pointer to containing structure pointer
 * 
 * Given a pointer to a list_elem_t field within a structure, this macro
 * calculates and returns a pointer to the containing structure.
 * 
 * @param elem Pointer to the list_elem_t field
 * @param container Type name of the containing structure
 * @param field Name of the list_elem_t field within the container
 * @return Pointer to the containing structure
 * 
 * @note This is similar to the Linux kernel's container_of macro
 * 
 * @example
 * struct task {
 *     int pid;
 *     list_elem_t list_node;
 * };
 * list_elem_t* elem = get_some_element();
 * task* t = LIST_CONTAINER(elem, task, list_node);
 */
#define LIST_CONTAINER(elem, container, field)                                      \
	((container*)((uintptr_t)(elem) - __builtin_offsetof(container, field)))