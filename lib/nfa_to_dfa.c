/*
 * Conversion of nfa to dfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nfa_to_dfa.h"
#include "simple_list.h"

#define STATE_HARD_LIMIT	(1000000)
#define DEBUG_FREQ		(1000)

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

int size_t_cmp(void *c1, void *c2)
{
	if (((size_t *)c1)[0] < ((size_t *)c2)[0])
		return -1;
	else if (((size_t *)c1)[0] > ((size_t *)c2)[0])
		return 1;

	return 0;
}

struct slist_description slist_description_size_t = {
	.name		= "slist - size_t",
	.element_size	= sizeof(size_t),
	.element_alloc	= NULL,
	.element_free	= NULL,
	.isorder	= 1,
	.element_cmp	= size_t_cmp,
	.type		= SLIST_ARRAY
};

int state_alloc(void *ptr)
{
	slist_set_type(ptr, size_t);
	return slist_alloc(ptr);
}

void state_free(void *ptr)
{
	slist_free(ptr);
}

int state_cmp(void *c1, void *c2)
{
	struct slist	*l1 = c1,
			*l2 = c2;

	for (size_t i = 0; i < min(l1->elem_cnt, l2->elem_cnt); i++) {
		int cmp = l1->desc->element_cmp(slist_get_element(l1, i),
						slist_get_element(l2, i)
						);
		if (cmp != 0)
			return cmp;
	}
	if (l1->elem_cnt == l2->elem_cnt)
		return 0;
	else if (l1->elem_cnt < l2->elem_cnt)
		return -1;
	else
		return 1;
}

struct slist_description slist_description_state = {
	.name		= "slist - state",
	.element_size	= sizeof(struct slist),
	.element_alloc	= NULL,
	.element_free	= state_free,
	.isorder	= 0,
	.element_cmp	= state_cmp,
	.type		= SLIST_ARRAY
};

struct slist_description slist_description_size_t_cache = {
	.name		= "slist - size_t (cache)",
	.element_size	= sizeof(size_t),
	.element_alloc	= NULL,
	.element_free	= NULL,
	.isorder	= 0,
	.element_cmp	= size_t_cmp,
	.type		= SLIST_ARRAY
};
struct slist_description slist_description_state_cache = {
	.name		= "slist - state (cache)",
	.element_size	= sizeof(struct slist),
	.element_alloc	= NULL,
	.element_free	= NULL,
	.isorder	= 0,
	.element_cmp	= state_cmp,
	.type		= SLIST_ARRAY
};

int convert_nfa_to_dfa(struct dfa *dst, struct nfa *src)
{
	int	ret = 0;
	struct slist	tmp, states, *ptr, states_cache, index_cache;
	struct nfa_node	*nptr;
	size_t	nindex;
	size_t	slindex = 0; /* self last state */
	size_t	sdindex = 0;
	slist_set_type(&states, state);
	slist_alloc(&states);
	slist_set_type(&tmp, size_t);
	slist_alloc(&tmp);
	slist_insert(&tmp, &(src->first_index), NULL);

	slist_insert(&states, &tmp, NULL);

	dfa_add_state(dst, &(dst->first_index));
	slist_set_type(&states_cache, state_cache);
	slist_set_type(&index_cache, size_t_cache);
	slist_alloc(&states_cache);
	slist_alloc(&index_cache);
	size_t	hit = 0;

	for (size_t i = 0; i < states.elem_cnt; i++) {
		if (slindex != 0 && i == slindex)
			continue;
		if (sdindex != 0 && i == sdindex)
			continue;
		ptr = slist_get_element(&states, i);

		int	to_loop = 0;

		for (size_t k = 0; k < ptr->elem_cnt && !to_loop; k++) {
			nindex = ((size_t *)slist_get_element(ptr, k))[0];
			if (src->nodes[nindex].self_closed && src->nodes[nindex].prefinal)
				to_loop = 1;
		}
		if (to_loop) {
			if (slindex == 0) {
				dfa_add_state(dst, &slindex);
				dfa_state_set_final(dst, slindex, 1);
				for (int j = 0; j < 256; j++)
					dfa_add_trans(dst, slindex, j, slindex);
				slist_alloc(&tmp);
				size_t	tmp_big = ~0;
				slist_insert(&tmp, &tmp_big, NULL);
				slist_insert(&states, &tmp, NULL);
			}

			for (int j = 0; j < 256; j++)
				dfa_add_trans(dst, i, j, slindex);

			continue;
		}

		for (unsigned int j = 0; j < 256; j++) {
		ptr = slist_get_element(&states, i);
			int end = 0, cnt = 0;
			slist_alloc(&tmp);
			for (size_t k = 0; k < ptr->elem_cnt; k++) {
				nindex = ((size_t *)slist_get_element(ptr, k))[0];
				nptr = &(src->nodes[nindex]);
				for (size_t l = 0; l < nptr->trans_cnt[j]; l++) {
					slist_insert(&tmp, &(nptr->trans[j][l]), NULL);
					if (src->nodes[nptr->trans[j][l]].isfinal)
						end = 1;
					cnt++;
				}
			}
			if (tmp.elem_cnt == 0) {
				if (sdindex == 0) {
					dfa_add_state(dst, &sdindex);
					dfa_state_set_final(dst, sdindex, 0);
					slist_insert(&states, &tmp, NULL);
					for (int k = 0; k < 256; k++)
						dfa_add_trans(dst, sdindex, k, sdindex);
				} else {
					slist_free(&tmp);
				}

				dfa_add_trans(dst, i, j, sdindex);
				continue;
			}

			size_t	t_index;
			size_t	f_index;
			if (slist_find_element(&f_index, &states_cache, &tmp) == 0) {
				t_index = ((size_t *)slist_get_element(&index_cache, f_index))[0];
				hit++;
				goto just_add;
			}


			if (slist_insert(&states, &tmp, &t_index) == 0) {
				if (states.elem_cnt != dst->state_cnt) {
					if (states.elem_cnt >= STATE_HARD_LIMIT) {
						ret = -1;
						goto out;
					}

					dfa_add_state(dst, &nindex);
					if (end)
						dfa_state_set_final(dst, nindex, 1);
					else
						dfa_state_set_final(dst, nindex, 0);
				} else {
					printf("some problems\n");
				}
				slist_insert(&states_cache, &tmp, NULL);
				slist_insert(&index_cache, &t_index, NULL);

				dfa_add_trans(dst, i, (unsigned char) j, t_index);
			} else {
				slist_insert(&states_cache, slist_get_element(&states, t_index), NULL);
				slist_insert(&index_cache, &t_index, NULL);
just_add:
				dfa_add_trans(dst, i, (unsigned char) j, t_index);
				slist_free(&tmp);
			}
		}
		if (i % DEBUG_FREQ == DEBUG_FREQ - 1)
			fprintf(stderr, "checked: %7zu/%zu\n", i + 1, states.elem_cnt);

		if (states_cache.elem_cnt > max(10000, states.elem_cnt / 100 + 100)) {
			fprintf(stderr, "cache: %zu, hits=%zu\n", states_cache.elem_cnt, hit);
			hit = 0;
			slist_free(&states_cache);
			slist_free(&index_cache);
			slist_alloc(&states_cache);
			slist_alloc(&index_cache);
		}
	}

out:
	slist_free(&states_cache);
	slist_free(&index_cache);

	slist_free(&states);

	dst->comment_size = src->comment_size;
	dst->comment = realloc(dst->comment, dst->comment_size);
	memcpy(dst->comment, src->comment, dst->comment_size);

	return ret;
}
