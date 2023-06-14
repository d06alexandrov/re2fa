/*
 * Definition of deterministic finite automaton.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <stdio.h>

#ifdef USE_ZLIB
#include <zlib.h>
#define DFA_ZLIB_CHUNK_SIZE	(4096 * 16)
#define DFA_ZLIB_LEVEL		(9)
#endif

#include "dfa.h"

#define DFA_CHUNK_SIZE		(32)

#define _ALIGN_TO(a, b)	((((a) + (b) - 1) / (b)) * (b))
#define _4CHAR_TO_UINT(a,b,c,d) (a + b * 256 + c * 256 * 256 + d * 256 * 256 * 256)

#define GET_BIT(a,b) (((a)[(b) / 8] >> ((b) % 8)) & 1)
#define SET_BIT(a,b) (a)[(b) / 8] |= (1 << ((b) % 8))

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static int max_to_bps(size_t max)
{
	int	res = 0;
	while (max >> res != 0 && res < 64)
		res += 8;
	switch (res) {
	case 0:
	case 8:
		return 8;
	case 16:
		return 16;
	case 24:
	case 32:
		return 32;
	case 40:
	case 48:
	case 56:
	case 64:
	default:
		return 64;
	}
}

uint64_t bps_to_max(int bps)
{
	switch (bps) {
	case 8:
		return 0xFF;
	case 16:
		return 0xFFFF;
	case 32:
		return 0xFFFFFFFF;
	case 64:
		return 0xFFFFFFFFFFFFFFFF;
	default:
		return 0;
	}
}

int dfa_alloc(struct dfa *dst)
{
	dst->comment_size = 0;
	dst->comment = NULL;
	dst->state_cnt = 0;
	dst->state_malloc_cnt = 0;
	dst->bps = 64;
	dst->state_size = 256 * ((dst->bps + 7) / 8);
	dst->state_max_cnt = ~0;
	dst->trans = NULL;
	dst->flags = NULL;
	dst->first_index = 0;

	dst->external = NULL;
	dst->external_size = 0;
	return 0;
}

int dfa_alloc2(struct dfa *dfa, size_t max_cnt)
{
	dfa->comment_size = 0;
	dfa->comment = NULL;
	dfa->state_cnt = 0;
	dfa->state_malloc_cnt = 0;
	dfa->bps = max_to_bps(max_cnt);
	dfa->state_size = 256 * ((dfa->bps + 7) / 8);
	dfa->state_max_cnt = max_cnt;
	dfa->trans = NULL;
	dfa->flags = NULL;
	dfa->first_index = 0;

	dfa->external = NULL;
	dfa->external_size = 0;
	return 0;
}

static int dfa_add_trans_native(struct dfa *dfa, size_t state_size, int bps, size_t from, unsigned char mark, size_t to);
static size_t dfa_get_trans_native(struct dfa *dfa, size_t state_size, int bps, size_t from, unsigned char mark);

int dfa_change_max_size(struct dfa *dfa, size_t max_cnt)
{
	if (dfa->state_cnt > max_cnt)
		return -1;

	if (dfa->bps == max_to_bps(max_cnt)) {
		dfa->state_max_cnt = max_cnt;
	} else {
		int	bps_new = max_to_bps(max_cnt);
		size_t	state_size_new = 256 * ((bps_new + 7) / 8);
		if (dfa->bps > bps_new) {
			for (size_t i = 0; i < dfa->state_cnt; i++)
				for (int j = 0; j < 256; j++) {
					size_t	to = dfa_get_trans(dfa, i, j);
					dfa_add_trans_native(dfa, state_size_new, bps_new, i, j, to);
				}
			dfa->trans = realloc(dfa->trans, state_size_new * dfa->state_malloc_cnt);
		} else {
			dfa->trans = realloc(dfa->trans, state_size_new * dfa->state_malloc_cnt);
			for (size_t i = dfa->state_cnt; i > 0; i--)
				for (int j = 255; j > -1; j--) {
					size_t	to = dfa_get_trans(dfa, i - 1, j);
					dfa_add_trans_native(dfa, state_size_new, bps_new, i - 1, j, to);
				}
		}

		dfa->bps = bps_new;
		dfa->state_size = state_size_new;
		dfa->state_max_cnt = max_cnt;
	}

	return 0;
}

void dfa_free(struct dfa *dfa)
{
	if (dfa != NULL) {
		free(dfa->trans);
		free(dfa->flags);
		free(dfa->comment);
		free(dfa->external);
	}
}

int dfa_copy(struct dfa *dst, struct dfa *src)
{
	*dst = *src;

	dst->comment = malloc(dst->comment_size);
	dst->trans = malloc(dst->state_size * dst->state_malloc_cnt);
	dst->flags = malloc(sizeof(*dst->flags) * dst->state_malloc_cnt);

	memcpy(dst->comment, src->comment, dst->comment_size);
	memcpy(dst->trans, src->trans, dst->state_size * dst->state_cnt);
	memcpy(dst->flags, src->flags, sizeof(*dst->flags) * dst->state_cnt);

	return 0;
}

int dfa_join(struct dfa *dst_g, struct dfa *src_g)
{
/* TODO! */
	struct dfa	*dst = dst_g, *src = src_g;
	if (dst_g->state_cnt < src_g->state_cnt) {
		dst = src_g;
		src = dst_g;
	}
	struct dfa	dfa_out;
	size_t	*pairs = malloc(sizeof(size_t) * 2);
	size_t	**pairs_2 = malloc(sizeof(size_t *) * dst->state_cnt);
	size_t	*pairs_cnt = malloc(sizeof(size_t) * dst->state_cnt);
	size_t	cnt = 0;

	memset(pairs, 0x00, sizeof(size_t *) * 2);
	memset(pairs_2, 0x00, sizeof(size_t *) * dst->state_cnt);
	for (size_t i = 0; i < dst->state_cnt; i++)
		pairs_cnt[i] = 0;


	size_t	*tmp_pairs = malloc(sizeof(size_t) * 3 * 256);
	size_t	tmp_cnt = 0;

	size_t	cur[2];
	size_t	next[2];

	pairs[0] = dst->first_index;
	pairs[1] = src->first_index;
	pairs_2[dst->first_index] = realloc(pairs_2[dst->first_index],
					  ++pairs_cnt[dst->first_index] * 2 * sizeof(size_t));
	pairs_2[dst->first_index][(pairs_cnt[dst->first_index] - 1) * 2] = src->first_index;
	pairs_2[dst->first_index][(pairs_cnt[dst->first_index] - 1) * 2 + 1] = cnt;
	cnt++;

	dfa_alloc(&dfa_out);
	dfa_add_state(&dfa_out, NULL);
	dfa_state_set_last(&dfa_out, 0, dfa_state_is_last(dst, dst->first_index) ||
				  dfa_state_is_last(src, src->first_index));
	for (size_t cur_index = 0; cur_index < cnt; cur_index++) {
		tmp_cnt = 0;
		cur[0] = pairs[cur_index * 2];
		cur[1] = pairs[cur_index * 2 + 1];

		if ((dfa_state_is_deadend(dst, cur[0]) && dfa_state_is_last(dst, cur[0])) ||
		    (dfa_state_is_deadend(src, cur[1]) && dfa_state_is_last(src, cur[1])) ||
		    (dfa_state_is_deadend(dst, cur[0]) && dfa_state_is_deadend(src, cur[1]))) {
			for (int i = 0; i < 256; i++)
				dfa_add_trans(&dfa_out, cur_index, i, cur_index);
			dfa_state_calc_deadend(&dfa_out, cur_index);
if (!dfa_state_is_last(&dfa_out, cur_index))
	printf("%s:%d ERROR\n", __FILE__, __LINE__);
			continue;
		}

		for (int i = 0; i < 256; i++) {
			int	found = 0;
			size_t	next_index;

			next[0] = dfa_get_trans(dst, cur[0], i);
//dst->nodes[cur[0]].trans[i];
			next[1] = dfa_get_trans(src, cur[1], i);
//src->nodes[cur[1]].trans[i];

			for (int j = 0; j < tmp_cnt; j++) {
				if (tmp_pairs[j * 3] == next[0] &&
				    tmp_pairs[j * 3 + 1] == next[1]) {
					found = 1;
					next_index = tmp_pairs[j * 3 + 2];
					break;
				}
			}

			if (!found) {
				for (size_t j = 0; j < pairs_cnt[next[0]]; j++)
					if (pairs_2[next[0]][j * 2] == next[1]) {
						found = 1;
						next_index = pairs_2[next[0]][j * 2 + 1];
						break;
					}

				if (!found) {
					next_index = cnt;
					pairs = realloc(pairs,
							sizeof(size_t) * 2 * ++cnt);
					pairs[next_index * 2] = next[0];
					pairs[next_index * 2 + 1] = next[1];
					pairs_2[next[0]] = realloc(pairs_2[next[0]], sizeof(size_t) * 2 * ++pairs_cnt[next[0]]);
					pairs_2[next[0]][(pairs_cnt[next[0]] - 1) * 2] = next[1];
					pairs_2[next[0]][(pairs_cnt[next[0]] - 1) * 2 + 1] = next_index;
					if (dfa_add_state(&dfa_out, NULL) != 0)
						printf("%d\n", __LINE__);

					dfa_state_set_last(&dfa_out, next_index, dfa_state_is_last(dst, next[0]) ||
							   dfa_state_is_last(src, next[1]));
				}

				tmp_pairs[tmp_cnt * 3] = next[0];
				tmp_pairs[tmp_cnt * 3 + 1] = next[1];
				tmp_pairs[tmp_cnt * 3 + 2] = next_index;
				tmp_cnt++;
			}

			dfa_add_trans(&dfa_out, cur_index, i, next_index);
		}
		dfa_state_calc_deadend(&dfa_out, cur_index);
	}
	for (size_t i = 0; i < dst->state_cnt; i++)
		free(pairs_2[i]);

	free(pairs_2);
	free(pairs);
	free(pairs_cnt);

	free(tmp_pairs);

/* copy comments to result dfa */

	dfa_out.comment_size = dst_g->comment_size + src_g->comment_size;
	dfa_out.comment = malloc(dfa_out.comment_size);
	memcpy(dfa_out.comment, dst_g->comment, dst_g->comment_size);
	memcpy(dfa_out.comment + dst_g->comment_size, src_g->comment,
						src_g->comment_size);
	if (dst_g->comment_size > 0 && src_g->comment_size > 0)
		dfa_out.comment[dst_g->comment_size - 1] = '\n';

/*******************************/

	dfa_free(dst_g);

	*dst_g = dfa_out;

	return 0;
}

int dfa_append(struct dfa *first, struct dfa *second)
{
	uint64_t	m_index = 0;
	uint8_t		m_found = 0;

	if (dfa_state_is_last(first, first->first_index)) {
		m_found = 1;
		m_index = first->first_index;
	}

	for (uint64_t i = 0; i < first->state_cnt && !m_found; i++)
		if (dfa_state_is_last(first, i)) {
				m_found = 1;
				m_index = i;
		}

	if (!m_found)
		return -1;

	uint64_t	offset;
	uint64_t	sf_index = second->first_index;
	dfa_add_n_state(first, second->state_cnt - 1, &offset);

	uint64_t	m_trans[256];
	int		m_is_last = dfa_state_is_last(second, sf_index);
	for (int a = 0; a < 256; a++) {
		uint64_t	real_to = dfa_get_trans(second, sf_index, a);
		if (real_to > sf_index)
			m_trans[a] = offset + real_to - 1;
		else if (real_to < sf_index)
			m_trans[a] = offset + real_to;
		else
			m_trans[a] = m_index;
	}

	for (uint64_t i = 0; i < first->state_cnt; i++)
		if (dfa_state_is_last(first, i)) {
			for (int a = 0; a < 256; a++)
				dfa_add_trans(first, i, a, m_trans[a]);
			dfa_state_set_last(first, i, m_is_last);
		}

	for (uint64_t i = 0; i < second->state_cnt; i++) {
		if (i == sf_index)
			continue;

		uint64_t	cur = offset + i;

		if (i > sf_index)
			cur--;

		int	is_last = dfa_state_is_last(second, i);
		dfa_state_set_last(first, cur, is_last);

		for (int a = 0; a < 256; a++) {
			uint64_t	real_to = dfa_get_trans(second, i, a),
					to;

			if (real_to > sf_index)
				to = offset + real_to - 1;
			else if (real_to < sf_index)
				to = offset + real_to;
			else
				to = m_index;

			dfa_add_trans(first, cur, a, to);
		}
	}

	dfa_minimize(first);

	return 0;
}


struct queue_size_t {
	size_t	cnt;
	struct queue_size_t_block	*first, *insert, *last;
};

static int queue_init(struct queue_size_t *);
static void queue_free(struct queue_size_t *);
static int queue_is_empty(struct queue_size_t *);
static size_t queue_pop(struct queue_size_t *);
static int queue_add(struct queue_size_t *, size_t);
static int queue_push(struct queue_size_t *queue, size_t element)
{
	return queue_add(queue, element);
}

#define GT_ITSELF	(0)
#define GT_PREIMG	(1)

int dfa_minimize(struct dfa *dst)
{
	size_t	*class_elements = NULL,	/* class -> element */
		*class_offset = NULL,	/* offsets of classes in class_elements*/
		*element_class = NULL;	/* element -> class (not as before) */
	char	*preimg_div = NULL;
	char	*is_done = NULL;
	struct {size_t size, cnt; size_t *mem;}	*class_preimage;

	size_t	max_cnt = dst->state_cnt,
		class_cnt = 2;

	class_elements = malloc(sizeof(size_t) * max_cnt);
	class_offset = malloc(sizeof(size_t) * 2 * max_cnt);
	element_class = malloc(sizeof(size_t) * max_cnt);

	preimg_div = malloc(max_cnt);
	memset(preimg_div, 0x00, max_cnt);
	is_done = malloc(max_cnt);
	memset(is_done, 0x00, max_cnt);

	class_preimage = malloc(sizeof(*class_preimage) * max_cnt);
	memset(class_preimage, 0x00, sizeof(*class_preimage) * max_cnt);

	class_offset[0] = 0;
	class_offset[1] = 0;

	int	first_is_last = dfa_state_is_last(dst, dst->first_index);

	for (size_t i = 0; i < max_cnt; i++)
		if (dfa_state_is_last(dst, i) == first_is_last) {
			class_elements[class_offset[1]++] = i;
			element_class[i] = 0;
		}

	class_offset[2] =
	class_offset[3] = class_offset[1];

	for (size_t i = 0; i < max_cnt; i++)
		if (dfa_state_is_last(dst, i) != first_is_last) {
			class_elements[class_offset[3]++] = i;
			element_class[i] = 1;
		}

	struct queue_size_t	queue;
	struct queue_size_t	tqueue; /* types of groups in queue */
	queue_init(&queue);
	queue_init(&tqueue);

	if (class_offset[3] == class_offset[1]) {
		printf("c_offset %zu %zu| %d\n", class_offset[1], class_offset[3], first_is_last);
		goto out;
	}

	queue_push(&queue, 0);
	queue_push(&queue, 1);
	queue_push(&tqueue, GT_ITSELF); /* 0 - after div itself, 1 - div of image */
	queue_push(&tqueue, GT_ITSELF);
	if (class_offset[1] - class_offset[0] == 1)
		is_done[0] = 1;
	if (class_offset[3] - class_offset[2] == 1)
		is_done[1] = 1;

	class_preimage[0].size = 2;
	class_preimage[0].cnt = 2;
	class_preimage[0].mem = malloc(sizeof(size_t) * class_preimage[0].size);
	class_preimage[1].size = 2;
	class_preimage[1].cnt = 2;
	class_preimage[1].mem = malloc(sizeof(size_t) * class_preimage[1].size);
	class_preimage[0].mem[0] =
	class_preimage[1].mem[0] = 0;
	class_preimage[0].mem[1] =
	class_preimage[1].mem[1] = 1;

	while (!queue_is_empty(&queue)) {
		size_t	cl_num = queue_pop(&queue);
		size_t	cl_type = queue_pop(&tqueue);

		if (is_done[cl_num]) {
			if (class_preimage[cl_num].cnt != 0) {
				class_preimage[cl_num].cnt = 0;
				if (class_preimage[cl_num].mem != NULL)
					free(class_preimage[cl_num].mem);
				class_preimage[cl_num].mem = NULL;
			}
			continue;
		}

		if (cl_type == GT_PREIMG) {
			if (preimg_div[cl_num] == 0)
				continue;
		}

		preimg_div[cl_num] = 0;

		size_t	cl_begin = class_offset[cl_num * 2],
			cl_end = class_offset[cl_num * 2 + 1],
			cl_dst = class_offset[cl_num * 2] + 1,
			cl_src = class_offset[cl_num * 2] + 1;
		size_t	cl_image[256];

		if (cl_end - cl_begin == 1) /* it's an one element class */ {
			is_done[cl_num] = 1;
			continue;
		}

		for (int i = 0; i < 256; i++)
			cl_image[i] = element_class[dfa_get_trans(dst, class_elements[cl_begin], i)];

		while (cl_src < cl_end) {
			int	diff = 0;
			for (int i = 0; i < 256 && diff == 0; i++)
				if (cl_image[i] != element_class[dfa_get_trans(dst, class_elements[cl_src], i)]) {
					diff++;
					break;
				}

			if (!diff) {
				if (cl_dst != cl_src) {
						size_t	s_tmp;
						s_tmp = class_elements[cl_dst];
						class_elements[cl_dst] = class_elements[cl_src];
						class_elements[cl_src] = s_tmp;
				}
				cl_dst++;
			}
			cl_src++;
		}

		if (cl_src == cl_dst) {
			/* no changes in class */
		} else {
			/* changed -> add to queue dependencies */
			for (size_t i = 0; i < class_preimage[cl_num].cnt; i++) {
				size_t	i_class = class_preimage[cl_num].mem[i];
				if (class_offset[i_class * 2 + 1] - class_offset[i_class * 2] > 1 &&
				    preimg_div[i_class] == 0 &&
				    !is_done[i_class]) {
					queue_push(&queue, i_class);
					queue_push(&tqueue, GT_PREIMG);
					preimg_div[i_class] = 1;
				}
			}
			class_preimage[cl_num].cnt = 0;

			class_offset[cl_num * 2 + 1] = cl_dst;
			class_offset[class_cnt * 2] = cl_dst;
			class_offset[class_cnt * 2 + 1] = cl_end;

			for (size_t i = class_offset[class_cnt * 2];
			     i < class_offset[class_cnt * 2 + 1]; i++) {
				element_class[class_elements[i]] = class_cnt;
			}


			if (cl_end - cl_dst != 1) {
			/* add to queue */
				queue_push(&queue, class_cnt);
				queue_push(&tqueue, GT_ITSELF);
			} else {
				is_done[class_cnt] = 1;
			}

			class_cnt++;

			if (cl_dst - cl_begin != 1) {
				queue_push(&queue, cl_num);
				queue_push(&tqueue, GT_ITSELF);
			} else {
				is_done[cl_num] = 1;
				free(class_preimage[cl_num].mem);
				class_preimage[cl_num].mem = NULL;
			}
		}

		if (class_offset[cl_num * 2 + 1] - class_offset[cl_num * 2] != 1) {
			/* add dependencies to image of main class */
			size_t	cl_subimage[256], cl_subimage_cnt = 0;
			for (int i = 0; i < 256; i++) {
				if (class_offset[cl_image[i] * 2 + 1] - class_offset[cl_image[i] * 2] == 1 ||
				    is_done[cl_image[i]])
					continue;
				int	j = 0;
				while (j < cl_subimage_cnt) {
					if (cl_subimage[j] == cl_image[i])
						break;
					j++;
				}

				if (j == cl_subimage_cnt)
					cl_subimage[cl_subimage_cnt++] = cl_image[i];
			}

			if (cl_subimage_cnt == 0)
				is_done[cl_num] = 1;

			if (cl_subimage_cnt == 1 && cl_src == cl_dst && cl_subimage[0] == cl_num)
				is_done[cl_num] = 1;

			if (!is_done[cl_num])
			for (int i = 0; i < cl_subimage_cnt; i++) {
				size_t	i_class = cl_subimage[i];

				size_t	j = 0;
				while (j < class_preimage[i_class].cnt) {
					if (class_preimage[i_class].mem[j] == cl_num)
						break;
					j++;
				}

				if (j == class_preimage[i_class].cnt) {
					if (class_preimage[i_class].cnt == class_preimage[i_class].size) {
						class_preimage[i_class].size++;
						class_preimage[i_class].mem = realloc(class_preimage[i_class].mem,
										      sizeof(size_t) * class_preimage[i_class].size);
					}
					class_preimage[i_class].mem[class_preimage[i_class].cnt++] = cl_num;
				}
			}
		}
	}

	if (element_class[dst->first_index] != 0) {
		size_t	cl_tmp = element_class[dst->first_index];
		for (size_t i = 0; i < max_cnt; i++)
			if (element_class[i] == 0)
				element_class[i] = cl_tmp;
			else if (element_class[i] == cl_tmp)
				element_class[i] = 0;
	}

/* using class_elements as temporary storage of original element_class */
	memcpy(class_elements, element_class, sizeof(size_t) * max_cnt);
/* now element class can be changed, but class_elements is permanent */
	for (size_t i = 0; i < class_cnt; i++)
		for (size_t j = i; j < max_cnt; j++)
			if (element_class[j] == i) {
				if (element_class[i] > i) {
					for (unsigned int a = 0; a < 256; a++) {
						size_t	tr_tmp;
						tr_tmp = dfa_get_trans(dst, i, a);
						dfa_add_trans(dst, i, a, class_elements[dfa_get_trans(dst, j, a)]);
						dfa_add_trans(dst, j, a, tr_tmp);
					}
					size_t	opt_tmp;
					opt_tmp = dfa_state_is_last(dst, i);
					dfa_state_set_last(dst, i, dfa_state_is_last(dst, j));
					dfa_state_set_last(dst, j, opt_tmp);
				size_t	cl_tmp;
				cl_tmp = element_class[j];
				element_class[j] = element_class[i];
				element_class[i] = cl_tmp;
				} else {
					for (int a = 0; a < 256; a++) {
						size_t	tr_tmp = class_elements[dfa_get_trans(dst, j, a)];
						dfa_add_trans(dst, i, a, tr_tmp);
					}
					dfa_state_set_last(dst, i, dfa_state_is_last(dst, j));
//					dfa_state_calc_deadend(dst, i);
				}

				break;
			}


	dst->state_cnt	= class_cnt;
//	dst->node_mem_size = (class_cnt + DFA_CHUNK_SIZE - 1) / DFA_CHUNK_SIZE;
//	dst->node_mem_size *= DFA_CHUNK_SIZE;
//	dst->nodes = realloc(dst->nodes, sizeof(struct dfa_node) * dst->node_mem_size);
	dst->first_index = 0;

	for (size_t i = 0; i < dst->state_cnt; i++)
		dfa_state_calc_deadend(dst, i);

out:
	for (size_t i = 0; i < class_cnt; i++)
		free(class_preimage[i].mem);
	free(class_preimage);
	queue_free(&queue);
	queue_free(&tqueue);

	free(class_elements);
	free(class_offset);
	free(element_class);

	free(is_done);

	free(preimg_div);

	return 0;
}

int dfa_compress(struct dfa *dfa)
{
	if (dfa->bps > max_to_bps(dfa->state_cnt))
		dfa_change_max_size(dfa, bps_to_max(max_to_bps(dfa->state_cnt)));

	return 0;
}

int dfa_add_trans_native(struct dfa *dfa, size_t state_size, int bps, size_t from, unsigned char mark, size_t to)
{
	void	*state;
	state = (char *)dfa->trans + from * state_size;

	switch (bps) {
	case 8:
		((uint8_t *)state)[mark] = to;
		break;
	case 16:
		((uint16_t *)state)[mark] = to;
		break;
	case 32:
		((uint32_t *)state)[mark] = to;
		break;
	case 64:
		((uint64_t *)state)[mark] = to;
		break;
	default:
		return -1;
	}

	return 0;
}

int dfa_add_trans(struct dfa *dfa, size_t from, unsigned char mark, size_t to)
{
	int	out = dfa_add_trans_native(dfa, dfa->state_size, dfa->bps,
					   from, mark, to);

	return out;
}

size_t dfa_get_trans_native(struct dfa *dfa, size_t state_size, int bps, size_t from, unsigned char mark)
{
	size_t	out = 0;
	void	*state;
	state = (char *)dfa->trans + from * state_size;

	switch (bps) {
	case 8:
		out = ((uint8_t *)state)[mark];
		break;
	case 16:
		out = ((uint16_t *)state)[mark];
		break;
	case 32:
		out = ((uint32_t *)state)[mark];
		break;
	case 64:
		out = ((uint64_t *)state)[mark];
		break;
	default:
		break;
	}

	return out;
}

size_t dfa_get_trans(struct dfa *dfa, size_t from, unsigned char mark)
{
	size_t	out = dfa_get_trans_native(dfa, dfa->state_size, dfa->bps,
					   from, mark);

	return out;
}

int dfa_state_is_last(struct dfa *dfa, size_t state)
{
	if (dfa->flags[state] & DFA_FLAG_LAST)
		return 1;
	else
		return 0;
}

int dfa_state_set_last(struct dfa *dfa, size_t state, int last)
{
	if (state > dfa->state_cnt)
		return -1;

	if (last)
		dfa->flags[state] |= DFA_FLAG_LAST;
	else
		dfa->flags[state] &= 0xFF ^ DFA_FLAG_LAST;

	return 0;
}

int dfa_state_is_deadend(struct dfa *dfa, size_t state)
{
	if (state > dfa->state_cnt)
		return -1;

	if (dfa->flags[state] & DFA_FLAG_DEADEND)
		return 1;
	else
		return 0;
}

int dfa_state_set_deadend(struct dfa *dfa, size_t state, int deadend)
{
	if (state > dfa->state_cnt)
		return -1;

	if (deadend)
		dfa->flags[state] |= DFA_FLAG_DEADEND;
	else
		dfa->flags[state] &= 0xFF ^ DFA_FLAG_DEADEND;

	return 0;
}

int dfa_state_calc_deadend(struct dfa *dfa, size_t state)
{
	int	is_deadend = 1;

	for (int i = 0; i < 256; i++)
		if (dfa_get_trans(dfa, state, i) != state) {
			is_deadend = 0;
			break;
		}

	dfa_state_set_deadend(dfa, state, is_deadend);

	return 0;
}

int dfa_add_state(struct dfa *dfa, size_t *index)
{
	if (dfa->state_cnt >= dfa->state_max_cnt)
		return -1;

	if (dfa->state_malloc_cnt == dfa->state_cnt) {
		dfa->state_malloc_cnt = MIN(dfa->state_malloc_cnt + DFA_CHUNK_SIZE,
					    dfa->state_max_cnt);

		dfa->trans = realloc(dfa->trans, dfa->state_size * dfa->state_malloc_cnt);
		dfa->flags = realloc(dfa->flags, dfa->state_malloc_cnt * sizeof(*dfa->flags));
	}

	if (index != NULL)
		*index = dfa->state_cnt;

	dfa->flags[dfa->state_cnt] = 0x00;

	dfa->state_cnt++;

	return 0;
}

int dfa_add_n_state(struct dfa *dfa, size_t cnt, size_t *index)
{
	if (index != NULL)
		*index = dfa->state_cnt;

	for (size_t i = 0; i < cnt; i++)
		dfa_add_state(dfa, NULL);

	return 0;
}

int dfa_save_to_file(struct dfa *src, char *filename)
{
	FILE	*dst = fopen(filename, "w");

	fwrite("\x57""DFA\x16\x16\x16\x16", 8, 1, dst);
	fwrite("ver#", 4, 1, dst);
	fwrite("\x00\x01\x00\x02", 4, 1, dst);
	fwrite("cnt#", 4, 1, dst);
	uint64_t	tmp64;
	tmp64 = src->state_cnt;
	fwrite(&tmp64, sizeof(tmp64), 1, dst);
	uint32_t	tmp32;
	tmp32 = src->bps;
	fwrite(&tmp32, sizeof(tmp32), 1, dst);

	fwrite("fst#", 4, 1, dst);

	fwrite(&(src->first_index), sizeof(src->first_index), 1, dst);

	tmp64 = src->comment_size;
	fwrite(&tmp64, sizeof(tmp64), 1, dst);
	fwrite(src->comment, 1, src->comment_size, dst);

#ifdef USE_ZLIB
	fwrite("alg:gzip", 8, 1, dst);
#else
	fwrite("alg:flat", 8, 1, dst);
#endif

	unsigned char in[256 * 8 + 8];
#ifdef USE_ZLIB
	unsigned char out[DFA_ZLIB_CHUNK_SIZE];
	int	zret;

	z_stream	zstrm;

	zstrm.zalloc = Z_NULL;
	zstrm.zfree = Z_NULL;
	zstrm.opaque = Z_NULL;

	zret = deflateInit(&zstrm, DFA_ZLIB_LEVEL);
	if (zret != Z_OK)
		goto out_err;
#endif
	for (size_t i = 0; i < src->state_cnt; i++) {
#ifdef USE_ZLIB
		int	flush = (i == src->state_cnt - 1 ? Z_FINISH : Z_NO_FLUSH);
#endif
		in[0] = src->flags[i];

		for (int j = 0; j < 256; j++)
			((uint64_t *)in)[j + 1] = dfa_get_trans(src, i, j);
#ifdef USE_ZLIB
		zstrm.avail_in = sizeof(uint64_t) * (256 + 1);
		zstrm.next_in = in;

		do {
			zstrm.avail_out = DFA_ZLIB_CHUNK_SIZE;
			zstrm.next_out = out;

			zret = deflate(&zstrm, flush);
			/* check error (zret) */
			/* need check */
			fwrite(out, 1, DFA_ZLIB_CHUNK_SIZE - zstrm.avail_out, dst);
		} while (zstrm.avail_out == 0);
#else
		fwrite(in, 1, sizeof(uint64_t) * (256 + 1), dst);
#endif
	}

#ifdef USE_ZLIB
	deflateEnd(&zstrm);
out_err:
#endif

	fclose(dst);

	return 0;
}

/* rewrite! */

int dfa_load_from_file(struct dfa *dst, char *filename)
{
	FILE	*src = fopen(filename, "r");

	if (src == NULL) {
		perror(filename);
		return -1;
	}

	dfa_alloc(dst);

	unsigned char	buffer[8], *ptr = buffer;
	size_t		size;

	size = fread(buffer, 1, 8, src);
	if (size != 8 || strncmp("\x57""DFA", (char *)buffer, 4))
		goto out_err;
	size = fread(buffer, 1, 8, src);
	if (size != 8 || strncmp("ver#\x00\x01\x00\x02", (char *)buffer, 8))
		goto out_err;
	size = fread(buffer, 1, 4, src);
	if (size != 4 || strncmp("cnt#", (char *)buffer, 4))
		goto out_err;
	size = fread(buffer, 1, 8, src);
	if (size != 8)
		goto out_err;
	uint64_t	state_cnt = ((uint64_t *)ptr)[0];
	size = fread(buffer, 1, 4, src);
	if (size != 4)
		goto out_err;
	uint32_t	bps = ((uint32_t *)ptr)[0];

	dfa_change_max_size(dst, bps_to_max(bps));

	ptr = buffer;
	dfa_add_n_state(dst, state_cnt, NULL);

	size = fread(buffer, 1, 4, src);
	if (size != 4 || strncmp("fst#", (char *)buffer, 4))
		goto out_err;
	size = fread(buffer, 1, 8, src);
	if (size != 8)
		goto out_err;

	dst->first_index = ((uint64_t *)ptr)[0];


	uint64_t	tmp64;
	if (!fread(buffer, 8, 1, src))
		goto out_err;
	tmp64 = ((uint64_t *)ptr)[0];

	dst->comment_size = tmp64;
	dst->comment = malloc(tmp64);
	size = fread(dst->comment, 1, tmp64, src);
	if (size != tmp64)
		goto out_err;

	size = fread(buffer, 1, 8, src);
	if (size != 8)
		goto out_err;

	switch (_4CHAR_TO_UINT(buffer[4], buffer[5], buffer[6], buffer[7])) {
	case _4CHAR_TO_UINT('f','l','a','t'):
	{

/*		size_t	read = 0, total = 0;

		while ((read = fread(((void *)dst->nodes) + total, 1,
				     sizeof(dst->nodes[0]) * dst->node_cnt - total,
				     src)) != 0) {
			total += read;
			if (sizeof(dst->nodes[0]) * dst->node_cnt - total == 0)
				break;
		}

		if (sizeof(dst->nodes[0]) * dst->node_cnt - total != 0)
			goto out_err;

		break;*/
		goto out_err;
	}
#ifdef USE_ZLIB
	case _4CHAR_TO_UINT('g', 'z', 'i', 'p'):
	{
		int	zret;

		z_stream	zstrm;
		unsigned char	in[DFA_ZLIB_CHUNK_SIZE];
		unsigned char	out[sizeof(uint64_t) * (256 + 1)];

		zstrm.zalloc = Z_NULL;
		zstrm.zfree = Z_NULL;
		zstrm.opaque = Z_NULL;
		zstrm.avail_in = 0;
		zstrm.next_in = Z_NULL;

		zret = inflateInit(&zstrm);
		if (zret != Z_OK)
			goto out_err;

		size_t	done = 0, state = 0;
		do {
			zstrm.avail_in = fread(in, 1, DFA_ZLIB_CHUNK_SIZE, src);
			if (ferror(src)) {
				inflateEnd(&zstrm);
				goto out_err;
			}
			if (zstrm.avail_in == 0)
				break;
			zstrm.next_in = in;

			do {
				zstrm.avail_out = sizeof(uint64_t) * (256 + 1) - done;
				zstrm.next_out = out + done;
				zret = inflate(&zstrm, Z_NO_FLUSH);
				/* check zret */
				done = sizeof(uint64_t) * (256 + 1) - zstrm.avail_out;
				if (done == sizeof(uint64_t) * (256 + 1)) {
					dst->flags[state] = out[0];
					for (int j = 0; j < 256; j++)
						dfa_add_trans(dst, state, j, ((uint64_t *)out)[j + 1]);
					dfa_state_calc_deadend(dst, state);
					state++;
					done = 0;
				}

			} while (zstrm.avail_out == 0);


		} while (zret != Z_STREAM_END);

		inflateEnd(&zstrm);

		break;
	}
#endif
	default:
		goto out_err;
		break;
	};

	fclose(src);
	return 0;
out_err:
	dfa_free(dst);
	fclose(src);

	return -1;
}


void dfa_print_char(unsigned char a)
{
	if (isprint(a)) {
		if (a == '"' || a == '-' || a == '\\')
			printf("\\");
		printf("%c", a);
	} else {
		printf("0x%02X", a);
	}
}

void dfa_print(struct dfa *src)
{
	printf("#dfa states: %zu, first: %zu \n", src->state_cnt, src->first_index);
//	struct dfa_node	*node;

	for (size_t i = 0; i < src->state_cnt; i++) {
//		node = &(src->nodes[i]);
		printf("{node [shape = %s, %s]; \"%zu\";}\n",
		       (dfa_state_is_last(src, i) ? "doublecircle" : "circle"),
		       (i == src->first_index ? "style=bold" : ""),
			i);
	}

	for (size_t i = 0; i < src->state_cnt; i++) {
		struct to_and_symb {size_t to; unsigned char symb;}	qu[256];



		printf("#node[%zu]\n", i);
		for (unsigned int j = 0; j < 256; j++) {
			qu[j].to = dfa_get_trans(src, i, j);

//			qu[j].to = src->nodes[i].trans[j];
			qu[j].symb = (unsigned char) j;
		}

		for (unsigned int j = 0; j < 255; j++)
			for (unsigned int k = 0; k < 255 - j; k++) {
				if (qu[k].to > qu[k + 1].to ||
				    (qu[k].to == qu[k + 1].to && qu[k].symb > qu[k + 1].symb)) {

					struct to_and_symb	tmp;
					tmp = qu[k];
					qu[k] = qu[k + 1];
					qu[k + 1] = tmp;
				}
			}
		for (unsigned int j = 0; j < 256; j++) {
			int	symbs[256];
			memset(symbs, 0u, sizeof(symbs));
			unsigned int d_bnd, u_bnd;
			unsigned int	total_trans = 0;
			d_bnd = j;
			for (u_bnd = j; u_bnd < 256 && qu[d_bnd].to == qu[u_bnd].to; u_bnd++) {
				symbs[qu[u_bnd].symb] = 1;
				total_trans++;
			}
			if (total_trans > 256/2 && total_trans != 256) {
				for (unsigned int j = 0; j < 256; j++)
					symbs[j] = 1 - symbs[j];
			}

			unsigned int	ub, db;
			int	started = 0;
printf(" \"%zu\" -> \"%zu\" [label = \"", i, qu[d_bnd].to);
if (total_trans > 1)
	printf("[");
if (total_trans > 256/2 && total_trans != 256)
	printf("^");
			for (db = 0; db < 256;)
				if (symbs[db] == 0) {
					db++;
				} else {
					for (ub = db; ub < 256 && symbs[ub]; ub++);

					ub--;
if (db == ub) {
	dfa_print_char(db);
} else if (ub - db == 1) {
	dfa_print_char(db);
	dfa_print_char(ub);
} else {
	dfa_print_char(db);
	printf("-");
	if (isprint(ub)) {
		if (ub == '\\' || ub == '"' || ub == ']')
			printf("\\");
		printf("%c", ub);
	} else {
		printf("\\x%02X", ub);
	}
}



					if (!started) {
						started = 1;
					} else {
					}


					db = ub + 1;
				}



			j = u_bnd - 1;
if (total_trans > 1)
	printf("]");
printf("\"];\n");
		}

	}
}

#define QUEUE_BLOCK_SIZE	(4096 / sizeof(size_t) - 3)

struct queue_size_t_block {
	size_t	cnt, offset; /* number of elements and offset of the first */
	size_t	data[QUEUE_BLOCK_SIZE];
	struct queue_size_t_block	*next;
};

int queue_init(struct queue_size_t *queue)
{
	queue->cnt	= 0;
	queue->first	=
	queue->insert	=
	queue->last	= NULL;

	return 0;
}

void queue_free(struct queue_size_t *queue)
{
	struct queue_size_t_block	*ptr = queue->first,
					*next;
	while (ptr != NULL) {
		next = ptr->next;
		free(ptr);
		ptr = next;
	}
	queue_init(queue);
}

int queue_is_empty(struct queue_size_t *queue)
{
	if (queue->cnt == 0)
		return 1;
	else
		return 0;
}

size_t queue_pop(struct queue_size_t *queue)
{
	size_t	out;

	if (queue->cnt == 0)
		return 0;

	struct queue_size_t_block	*ptr = queue->first;

	out = ptr->data[ptr->offset];
	ptr->cnt--;
	ptr->offset++;
	queue->cnt--;

	if (ptr->cnt == 0)
		ptr->offset = 0;
	else
		return out;

	if (queue->cnt != 0) {
		queue->first		= ptr->next;
		queue->last->next	= ptr;
		queue->last		= ptr;
		ptr->next		= NULL;
	} else {
		queue->insert = ptr;
	}

	return out;
}

int queue_add(struct queue_size_t *queue, size_t element)
{
	if (queue->first == NULL) {
		queue->first	=
		queue->insert	=
		queue->last	= malloc(sizeof(struct queue_size_t_block));
		queue->first->cnt	=
		queue->first->offset	= 0;
		queue->first->next	= NULL;
	}

	if (queue->insert->offset + queue->insert->cnt == QUEUE_BLOCK_SIZE) {
		if (queue->insert == queue->last) {
			queue->last	= malloc(sizeof(struct queue_size_t_block));
			queue->last->cnt	=
			queue->last->offset	= 0;
			queue->last->next	= NULL;
			queue->insert->next = queue->last;
		}

		queue->insert = queue->insert->next;
	}

	queue->insert->data[queue->insert->offset + queue->insert->cnt] = element;

	queue->insert->cnt++;

	queue->cnt++;

	return 0;
}
