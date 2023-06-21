/*
 * Simple FIFO and ordered array.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef REFA_SIMPLE_LIST_H
#define REFA_SIMPLE_LIST_H

#include <stddef.h>

#define SLIST_ARRAY	(0)
#define SLIST_QUEUE	(1)

/**
 * structure that is used to describe array or queue object
 */
struct slist_description {
	/**
	 * name of the slist type
	 */
	const char *name;

	/**
	 * size of the one stored element
	 */
	size_t element_size;

	/**
	 * function to allocate one element
	 */
	int (*element_alloc)(void *);

	/**
	 * function to destroy one element
	 */
	void (*element_free)(void *);

	/**
	 * if elements are ordered
	 * 0 if not; 1 if in ascending order; -1 if in descending order
	 */
	int isorder;

	/**
	 * pointer to function that can compare two elements
	 */
	int (*element_cmp)(void *, void *);

	/**
	 * type of the data structure - array or queue
	 */
	int type;
};

/**
 * structure that represents array or queue
 */
struct slist {
	/**
	 * pointer to the data structure description
	 */
	struct slist_description *desc;

	/**
	 * pointer to storage of elements
	 */
	void *mem;

	/**
	 * pointer to array with ordered indexes
	 */
	void *mem_order;

	/**
	 * index of the first lement in the queue
	 */
	size_t q_first;

	/**
	 * number of stored elements
	 */
	size_t elem_cnt;

	/**
	 * total memory capacity
	 */
	size_t	elem_mem_size;
};

/* alloc and free MUST be called only AFTER slist_set_type() call */
int slist_alloc(struct slist *);
void slist_free(struct slist *);

/* get element #@2 from @1 slist, returns pointer to element or NULL */
/*
 * WARNING: address of element can (will) be changed after `insert' operation!
 * So you HAVE to use result of this function immediately
 */
void *slist_get_element(struct slist *, size_t);
/*
 * create new element in @1 slist, @2 pointer to new element's index
 * WARNING: works for SLIST_ARRAY only
 */
int slist_add_element(struct slist *, size_t *);
/* add element, pointed to @2 (by memcpy), to @1 list. saves index to @3
 * returns 0 if suceed, -1 on error, -2 if element exists
 */
int slist_insert(struct slist *, void *, size_t *);
/* returns 0 if element @3 exists in list @2 and saves to @1 it's index */
int slist_find_element(size_t *, struct slist *, void *);

void slist_debug_printf(struct slist *src);

#define slist_set_type(dst, tname)						\
	do {									\
		((struct slist *)(dst))->desc = &(slist_description_##tname);	\
	} while (0)


#endif /* REFA_SIMPLE_LIST_H */
