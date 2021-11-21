/*
 * Simple FIFO and ordered array.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef __SIMPLE_LIST_H
#define __SIMPLE_LIST_H
#include <stddef.h>

#define SLIST_ARRAY	(0)
#define SLIST_QUEUE	(1)

struct slist_description {
	const char	*name;
	size_t	element_size;	/* element size */
	int	(*element_alloc)(void *);	/* func to alloc element */
	void	(*element_free)(void *);	/* func to dispose element */

	int	isorder;	/* 1 -- asc, -1 -- desc, 0 -- none */
	int	(*element_cmp)(void *, void *);	/* func to compare elements */
	int	type;	/* queue or array */
};

struct slist {
	struct slist_description	*desc;

	void	*mem;		/* storage of slist */
	void	*mem_order;
	size_t	q_first;	/* number of first element in queue */

	size_t	elem_cnt;	/* how much elements is stored now */
	size_t	elem_mem_size;	/* how much elements can be in *mem */
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


#endif /* __SIMPLE_LIST_H */
