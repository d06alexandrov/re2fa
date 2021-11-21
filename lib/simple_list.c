/*
 * Simple FIFO and ordered array.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include <string.h>
#include <stdlib.h>

#include <stdio.h>

#include "simple_list.h"

#define SLIST_CHUNK_SIZE	(4)

static int slist_array_alloc(struct slist *dst);
static void slist_array_free(struct slist *dst);
static void *slist_array_get_element(struct slist *src, size_t index);
static int slist_array_add_element(struct slist *dst, size_t *index);
static int slist_array_insert(struct slist *dst, void *src, size_t *);
static int slist_array_find_element(size_t *dst, struct slist *src, void *c);
static void slist_array_debug_printf(struct slist *src);

static int slist_queue_alloc(struct slist *dst);
static void slist_queue_free(struct slist *dst);
static void *slist_queue_get_element(struct slist *src, size_t index);
static void *slist_queue_get_element_inner(struct slist *dst, size_t index);
static int slist_queue_add_element(struct slist *dst, size_t *index);
static int slist_queue_add_element_inner(struct slist *dst, size_t *index, int alloc);
static int slist_queue_insert(struct slist *dst, void *src, size_t *);
static int slist_queue_find_element(size_t *dst, struct slist *src, void *c);
//void slist_queue_debug_printf(struct slist *src);

int slist_alloc(struct slist *dst)
{
	switch (dst->desc->type) {
	case SLIST_ARRAY:
		return slist_array_alloc(dst);
		break;
	case SLIST_QUEUE:
		return slist_queue_alloc(dst);
		break;
	default:
		return -1;
	}
}
void slist_free(struct slist *dst)
{
	switch (dst->desc->type) {
	case SLIST_ARRAY:
		slist_array_free(dst);
		return;
	case SLIST_QUEUE:
		slist_queue_free(dst);
		return;
	default:
//		return -1;
		return;
	}
}
void *slist_get_element(struct slist *src, size_t index)
{
	switch (src->desc->type) {
	case SLIST_ARRAY:
		return slist_array_get_element(src, index);
		break;
	case SLIST_QUEUE:
		return slist_queue_get_element(src, index);
		break;
	default:
		return NULL;
	}
}
int slist_add_element(struct slist *dst, size_t *index)
{
	switch (dst->desc->type) {
	case SLIST_ARRAY:
		return slist_array_add_element(dst, index);
		break;
	case SLIST_QUEUE:
		return slist_queue_add_element(dst, index);
		break;
	default:
		return -1;
	}
}
int slist_insert(struct slist *dst, void *src, size_t *index)
{
	switch (dst->desc->type) {
	case SLIST_ARRAY:
		return slist_array_insert(dst, src, index);
		break;
	case SLIST_QUEUE:
		return slist_queue_insert(dst, src, index);
		break;
	default:
		return -1;
	}
}
int slist_find_element(size_t *dst, struct slist *src, void *c)
{
	switch (src->desc->type) {
	case SLIST_ARRAY:
		return slist_array_find_element(dst, src, c);
		break;
	case SLIST_QUEUE:
		return slist_queue_find_element(dst, src, c);
		break;
	default:
		return -1;
	}
}
void slist_debug_printf(struct slist *src)
{
	switch (src->desc->type) {
	case SLIST_ARRAY:
		slist_array_debug_printf(src);
		return;
	case SLIST_QUEUE:
	default:
		return;
	}
}


/* slist_array */
int slist_array_alloc(struct slist *dst)
{
	dst->mem		= NULL;
	dst->elem_cnt		=
	dst->elem_mem_size	= 0;
	return 0;
}

void slist_array_free(struct slist *dst)
{
	if (dst->desc->element_free != NULL) {
		for (size_t i = 0; i < dst->elem_cnt; i++) {
			dst->desc->element_free(slist_array_get_element(dst, i));
		}
	}

	free(dst->mem);

	dst->mem		= NULL;
	dst->elem_cnt		=
	dst->elem_mem_size	= 0;
}

void *slist_array_get_element(struct slist *src, size_t index)
{
	if (index >= src->elem_cnt)
		return NULL;

	return ((char *)src->mem + src->desc->element_size * index);
}

int slist_array_add_element(struct slist *dst, size_t *index)
{
	if (dst->elem_mem_size <= dst->elem_cnt) {
		dst->elem_mem_size += 8;
		dst->mem = realloc(dst->mem, dst->elem_mem_size * dst->desc->element_size);
	}

	if (index != NULL)
		*index = dst->elem_cnt;

	if (dst->desc->element_alloc != NULL)
		dst->desc->element_alloc(slist_array_get_element(dst, dst->elem_cnt++));
	else
		dst->elem_cnt++;

	return 0;
}

int slist_array_insert(struct slist *dst, void *src, size_t *index)
{
	size_t	i_index = 0;
	if (dst->desc->isorder) {
		while (i_index < dst->elem_cnt) {
			int	cmp = dst->desc->element_cmp(
						src,
						slist_array_get_element(dst, i_index)
					);

			if (cmp == 0) {
				if (index != NULL)
					*index = i_index;
				return -2;
			} else if (cmp < 0) {
				break;
			}

			i_index++;
		}
	} else {
		if (dst->desc->element_cmp != NULL) {
			for (size_t i = 0; i < dst->elem_cnt; i++) {
				int	cmp = dst->desc->element_cmp(
							src,
							slist_array_get_element(dst, i)
						);

				if (cmp == 0) {
					if (index != NULL)
						*index = i;
					return -2;
				}
			}
		}

		i_index = dst->elem_cnt;
	}

	if (index != NULL)
		*index = i_index;

	if (slist_array_add_element(dst, NULL) != 0) {
		return -1;
	}

	memmove((char *)dst->mem + (i_index + 1) * dst->desc->element_size,
		(char *)dst->mem + i_index * dst->desc->element_size,
		(dst->elem_cnt - i_index - 1) * dst->desc->element_size);

	memcpy((char *)dst->mem + i_index * dst->desc->element_size,
		src, dst->desc->element_size);

	return 0;
}

int slist_array_find_element(size_t *dst, struct slist *src, void *c)
{
	for (*dst = 0; *dst < src->elem_cnt; (*dst)++)
		if (src->desc->element_cmp(c, slist_array_get_element(src, *dst)) == 0)
			return 0;

	return -1;
}

void slist_array_debug_printf(struct slist *src)
{
	fprintf(stderr, "[%016lX]->[%016lX] [%zu of %zu]\n", (unsigned long) src, (unsigned long)src->mem, src->elem_cnt, src->elem_mem_size);
}


/*
 * mem--->element_array[SLIST_CHUNK_SIZE]
 *     |->...
 *     -->element_array[SLIST_CHUNK_SIZE]
 * mem_order:
 *  if we have in array order ... i1 i2 ... (indexes in mem's notation),
 *    then mem_order[i1] = i2
 */

int slist_queue_alloc(struct slist *dst)
{
	dst->mem		=
	dst->mem_order		= NULL;
	dst->q_first		= ~0;
	dst->elem_cnt		=
	dst->elem_mem_size	= 0;
	return 0;
}

void slist_queue_free(struct slist *dst)
{
	void	**mem = dst->mem;
	if (dst->elem_mem_size != 0) {
		if (dst->desc->element_free != NULL) {
			for (size_t i = 0; i < dst->elem_cnt; i++) {
				dst->desc->element_free(slist_queue_get_element_inner(dst, i));
//				dst->desc->element_free(mem[i / SLIST_CHUNK_SIZE] + dst->desc->element_size * (i % SLIST_CHUNK_SIZE));
			}
		}

		for (size_t i = 0; i < (dst->elem_mem_size + SLIST_CHUNK_SIZE - 1) / SLIST_CHUNK_SIZE; i++)
			free(mem[i]);
		free(mem);
	}

	dst->mem		=
	dst->mem_order		= NULL;
	dst->q_first		= ~0;
	dst->elem_cnt		=
	dst->elem_mem_size	= 0;
}


void *slist_queue_get_element(struct slist *src, size_t index)
{
	/* without order */
	return slist_queue_get_element_inner(src, index);
}

void *slist_queue_get_element_inner(struct slist *dst, size_t index)
{
	void	**mem = dst->mem;

	if (index >= dst->elem_mem_size)
		return NULL;

	return ((char *)(mem[index / SLIST_CHUNK_SIZE]) +
		dst->desc->element_size * (index % SLIST_CHUNK_SIZE));
}

int slist_queue_add_element(struct slist *dst, size_t *index)
{
	return slist_queue_add_element_inner(dst, index, 1);
}

int slist_queue_add_element_inner(struct slist *dst, size_t *index, int alloc)
{
	if (dst->elem_mem_size <= dst->elem_cnt) {
		dst->elem_mem_size += SLIST_CHUNK_SIZE;
/*		if (dst->elem_mem_size == SLIST_CHUNK_SIZE) {
			dst->mem = malloc(sizeof(void *));
			dst->mem_order = malloc(sizeof(size_t) * dst->elem_mem_size);
		} else {
			dst->mem = realloc(dst->mem, sizeof(void *) * ((dst->elem_mem_size + SLIST_CHUNK_SIZE - 1) / SLIST_CHUNK_SIZE));
		}
*/
		dst->mem = realloc(dst->mem, sizeof(void *) * ((dst->elem_mem_size + SLIST_CHUNK_SIZE - 1) / SLIST_CHUNK_SIZE));
		dst->mem_order = realloc(dst->mem_order, sizeof(size_t) * dst->elem_mem_size);

		((void **)dst->mem)[(dst->elem_mem_size - 1) / SLIST_CHUNK_SIZE] = malloc(dst->desc->element_size * SLIST_CHUNK_SIZE);
	}

	if (index != NULL)
		*index = dst->elem_cnt;

	if (dst->desc->element_alloc != NULL && alloc)
		dst->desc->element_alloc(slist_queue_get_element_inner(dst, dst->elem_cnt));

	dst->elem_cnt++;

	return 0;
}

int slist_queue_insert(struct slist *dst, void *src, size_t *index)
{
	if (dst->elem_cnt == 0) {
		size_t	i_index;
		slist_queue_add_element_inner(dst, &i_index, 0);
		dst->q_first = i_index;
		((size_t *)dst->mem_order)[i_index] = ~0;
		memcpy(slist_queue_get_element_inner(dst, i_index),
		       src,
		       dst->desc->element_size);
		if (index != NULL)
			*index = i_index;
	} else {
		size_t	i_index_a = dst->q_first,
			i_index_b = dst->q_first,
			i_index;
		for (size_t i = 0; i < dst->elem_cnt; i++) {
			int	cmp;
			cmp = dst->desc->element_cmp(src,
						     slist_queue_get_element_inner(dst, i_index_a));

			if (cmp == 0) {
				if (index != NULL)
					*index = i_index_a;
				return -2;
			} else if (cmp < 0) {
				break;
			}

			i_index_b = i_index_a;
			i_index_a = ((size_t *)dst->mem_order)[i_index_a];
		}

		slist_queue_add_element_inner(dst, &i_index, 0);
		memcpy(slist_queue_get_element_inner(dst, i_index),
		       src,
		       dst->desc->element_size);

		if (i_index_b == dst->q_first && i_index_a == dst->q_first) {
			/* add to begin */
			dst->q_first = i_index;
			((size_t *)dst->mem_order)[i_index] = i_index_b;
		} else {
			/* here i_index_b < src < i_index_a */
			((size_t *)dst->mem_order)[i_index_b] = i_index;
			((size_t *)dst->mem_order)[i_index] = i_index_a;
		}
		if (index != NULL)
			*index = i_index;
	}

	return 0;
}

int slist_queue_find_element(size_t *dst, struct slist *src, void *c)
{
	/* without order */
	size_t	f_index = src->q_first;

	for (size_t i = 0; i < src->elem_cnt; i++) {
		int	cmp;
		cmp = src->desc->element_cmp(c,
					     slist_queue_get_element(src, f_index));
		if (cmp == 0) {
			*dst = f_index;
			return 0;
		} else if (cmp < 0) {
			return -1;
		}
		f_index = ((size_t *)src->mem_order)[f_index];
	}

	return -1;
}

//void slist_queue_debug_printf(struct slist *src);
