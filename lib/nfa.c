/*
 * Declaration of nondeterministic finite automaton.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <stdio.h>

#include "nfa.h"

#define NFA_CHUNK_SIZE		(4096 / sizeof(struct nfa_node) > 0	\
				 ? 4096 / sizeof(struct nfa_node)	\
				 : 8)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef MAX
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#endif

static int nfa_lambda_reachable(size_t *dst, size_t *cnt, size_t from, struct nfa *src);
static int nfa_trans_reachable(size_t *dst, size_t *cnt, size_t from, struct nfa *src);
static void nfa_print(struct nfa *);
/* @2th node initialization */
static int nfa_node_alloc(struct nfa_node *, size_t);
/* node clear */
static void nfa_node_free(struct nfa_node *);
/* add λ-transition */
static int nfa_node_add_lambda_trans(struct nfa_node *, size_t);
/* add regular transition */
static int nfa_node_add_trans(struct nfa_node *, unsigned char, size_t);
/* remove regular transition */
static int nfa_node_remove_trans(struct nfa_node *, unsigned char, size_t);
/* remove all transitions from node */
static int nfa_node_remove_trans_all(struct nfa_node *);

int nfa_alloc(struct nfa *nfa)
{
	nfa->comment = NULL;
	nfa->comment_size = 0;
	nfa->node_cnt = 0;
	nfa->node_mem_size = 0;
	nfa->nodes = NULL;
	nfa->first_index = 0;
	return 0;
}

void nfa_free(struct nfa *nfa)
{
	if (nfa != NULL) {
		for (size_t i = 0; i < nfa->node_cnt; i++)
			nfa_node_free(&(nfa->nodes[i]));
		free(nfa->nodes);
		free(nfa->comment);
	}
}

size_t nfa_state_count(struct nfa *nfa)
{
	return nfa->node_cnt;
}

size_t nfa_get_initial_state(struct nfa *nfa)
{
	return nfa->first_index;
}

int nfa_set_initial_state(struct nfa *nfa, size_t index)
{
	if (index >= nfa_state_count(nfa)) {
		return -1;
	}

	nfa->first_index = index;

	return 0;
}

int nfa_rebuild(struct nfa *nfa)
{
	nfa_remove_lambda(nfa);

	struct nfa fa;

	size_t		*tmp1 = malloc(sizeof(size_t) * (nfa->node_cnt + 1)),
			*tmp2 = malloc(sizeof(size_t) * (nfa->node_cnt + 1)),
			tmp_cnt, tmp_cnt2,
			magic_num = nfa->node_cnt + 1;

	nfa_alloc(&fa);

	nfa_trans_reachable(tmp1, &tmp_cnt, nfa->first_index, nfa);
	/* nodes in `tmp' are in depth order */
	for (size_t i = 0; i < nfa->node_cnt; i++) {
		tmp2[i] = magic_num;
		for (size_t j = 0; j < tmp_cnt; j++)
			if (tmp1[j] == i) {
				tmp2[i] = j;
				break;
			}
	}

	for (size_t i = tmp_cnt - 1; i > 0; i--) {
		if (nfa->nodes[tmp1[i]].isfinal)
			continue;

		int	remove = 1;

		for (unsigned int j = 0; j < 256 && remove; j++) {
			for (size_t k = 0; k < nfa->nodes[tmp1[i]].trans_cnt[j] && remove; k++) {
				if (tmp2[nfa->nodes[tmp1[i]].trans[j][k]] != magic_num)
					remove = 0;
			}
		}
		if (remove)
			tmp1[i] = magic_num;
	}

	tmp_cnt2 = 0;
	for (size_t i = 0; i < tmp_cnt; i++)
		if (tmp1[i] == magic_num)
			for (size_t j = i + 1; j < tmp_cnt; j++)
				if (tmp1[j] != magic_num) {
					tmp1[i] = tmp1[j];
					tmp1[j] = magic_num;
					break;
				}

	for (size_t i = 0; i < tmp_cnt; i++)
		if (tmp1[i] != magic_num)
			tmp_cnt2++;

	for (size_t i = 0; i < nfa->node_cnt; i++) {
		tmp2[i] = magic_num;
		for (size_t j = 0; j < tmp_cnt2; j++)
			if (tmp1[j] == i) {
				tmp2[i] = j;
				break;
			}
	}

	for (size_t i = 0; i < tmp_cnt2; i++)
		nfa_add_node(&fa, NULL);

	for (size_t i = 0; i < nfa->node_cnt; i++)
		if (tmp2[i] != magic_num) {
			/* copy nfa->nodes[i] to fa.nodes[tmp2[i]] */
			fa.nodes[tmp2[i]].isfinal = nfa->nodes[i].isfinal;

			for (unsigned int j = 0; j < 256; j++) {
				for(size_t k = 0; k < nfa->nodes[i].trans_cnt[j]; k++) {
					if (tmp2[nfa->nodes[i].trans[j][k]] != magic_num) {
						nfa_add_trans(&fa, tmp2[i], (unsigned char) j, tmp2[nfa->nodes[i].trans[j][k]]);
					}
				}
			}


		}

	for (size_t i = 0; i < fa.node_cnt; i++) {
		int	prefinal = 1, self_closed = 1;
		for (int j = 0; j < 256; j++) {
			int	tmp1 = 0, tmp2 = 0;
			for(size_t k = 0; k < fa.nodes[i].trans_cnt[j]; k++) {
				size_t	to = fa.nodes[i].trans[j][k];
				if (to == i)
					tmp1 = 1;
				if (fa.nodes[to].isfinal)
					tmp2 = 1;
				if (tmp1 && tmp2)
					break;
			}

			if (!tmp1)
				self_closed = 0;
			if (!tmp2)
				prefinal = 0;

			if (!prefinal && !self_closed)
				break;
		}

		fa.nodes[i].self_closed = self_closed;
		fa.nodes[i].prefinal = prefinal;
	}

	fa.comment = nfa->comment;
	fa.comment_size = nfa->comment_size;
	nfa->comment = NULL;
	nfa->comment_size = 0;

	nfa_free(nfa);
	*nfa = fa;

	free(tmp1);
	free(tmp2);

	return 0;
}

int nfa_join(struct nfa *dst, struct nfa *src)
{
	size_t	offset;

	if (src->node_cnt == 0)
		return 0;

	if (nfa_add_node_n(dst, src->node_cnt, &offset) != 0)
		return -1;

	nfa_add_lambda_trans(dst, dst->first_index, offset + src->first_index);

	for (size_t i = 0; i < src->node_cnt; i++) {
		dst->nodes[offset + i].isfinal = src->nodes[i].isfinal;

		for (size_t j = 0; j < src->nodes[i].lambda_cnt; j++)
			nfa_add_lambda_trans(dst, offset + i,
					offset + src->nodes[i].lambda_trans[j]);

		for (size_t j = 0; j < 256; j++)
			for (size_t k = 0; k < src->nodes[i].trans_cnt[j]; k++)
				nfa_add_trans(dst, offset + i,
					      (unsigned char) j,
					      offset + src->nodes[i].trans[j][k]);
	}

	offset = dst->comment_size;
	dst->comment_size += src->comment_size;
	dst->comment = realloc(dst->comment, dst->comment_size);
	memcpy(dst->comment + offset, src->comment, src->comment_size);
	if (offset > 0 && src->comment_size > 0)
		dst->comment[offset - 1] = '\n';

	return 0;
}

int nfa_add_node(struct nfa *nfa, size_t *index)
{
	if (nfa->node_mem_size == nfa->node_cnt) {
		void *tmp = nfa->nodes;
		nfa->node_mem_size += NFA_CHUNK_SIZE;
		nfa->nodes = malloc(nfa->node_mem_size * sizeof(struct nfa_node));
		memcpy(nfa->nodes, tmp, nfa->node_cnt * sizeof(struct nfa_node));
		free(tmp);
	}

	if (index != NULL)
		*index = nfa->node_cnt;

	nfa_node_alloc(&(nfa->nodes[nfa->node_cnt]), nfa->node_cnt);

	nfa->node_cnt++;

	return 0;
}

int nfa_add_node_n(struct nfa *nfa, size_t cnt, size_t *index)
{
	/* TEMPORARY implementation */
	if (cnt == 0)
		return -1;
	nfa_add_node(nfa, index);
	for (size_t i = 1; i < cnt; i++)
		nfa_add_node(nfa, NULL);
	return 0;
}

int nfa_state_is_final(struct nfa *nfa, size_t state)
{
	if (nfa->nodes[state].isfinal)
		return 1;
	else
		return 0;
}

int nfa_state_set_final(struct nfa *nfa, size_t state, int last)
{
	if (state > nfa->node_cnt)
		return -1;

	if (last)
		nfa->nodes[state].isfinal = 1;
	else
		nfa->nodes[state].isfinal = 0;

	return 0;
}

int nfa_add_lambda_trans(struct nfa *nfa, size_t from, size_t to)
{
	struct nfa_node *node = &(nfa->nodes[from]);

	if (MAX(from, to) >= nfa->node_cnt)
		return -1;

	nfa_node_add_lambda_trans(node, to);

	return 0;
}

size_t nfa_get_lambda_trans(struct nfa *nfa, size_t from, size_t **trans)
{
	if (from >= nfa_state_count(nfa)) {
		return 0;
	}

	if (trans != NULL) {
		*trans = nfa->nodes[from].lambda_trans;
	}

	return nfa->nodes[from].lambda_cnt;
}

int nfa_add_trans(struct nfa *dst, size_t from, unsigned char mark, size_t to)
{
	struct nfa_node *node = &(dst->nodes[from]);

	if (MAX(from, to) >= dst->node_cnt)
		return -1;

	nfa_node_add_trans(node, mark, to);

	return 0;
}

size_t nfa_get_trans(struct nfa *nfa, size_t from, unsigned char mark,
		     size_t **trans)
{
	if (from >= nfa_state_count(nfa)) {
		return 0;
	}

	if (trans != NULL) {
		*trans = nfa->nodes[from].trans[mark];
	}

	return nfa->nodes[from].trans_cnt[mark];
}

int nfa_remove_trans(struct nfa *nfa, size_t from, unsigned char mark, size_t to)
{
	if (MAX(from, to) >= nfa->node_cnt)
		return -1;

	struct nfa_node *node = &(nfa->nodes[from]);

	return nfa_node_remove_trans(node, mark, to);
}

int nfa_remove_trans_all(struct nfa *nfa, size_t from)
{
	if (from >= nfa->node_cnt)
		return -1;

	struct nfa_node	*node = &nfa->nodes[from];

	nfa_node_remove_trans_all(node);

	return 0;
}

int nfa_copy_trans(struct nfa *nfa, size_t to, size_t from)
{
	struct nfa_node	*from_node = &(nfa->nodes[from]),
			*to_node = &(nfa->nodes[to]);

	if (MAX(from, to) >= nfa->node_cnt)
		return -1;

	for (int i = 0; i < 256; i++) {
		for (size_t j = 0; j < from_node->trans_cnt[i]; j++) {
			nfa_node_add_trans(to_node, (unsigned char) i,
					   from_node->trans[i][j]);
		}
	}

	return 0;
}

int nfa_remove_lambda(struct nfa *nfa)
{
	int	isneed = 0;
	for (size_t i = 0; i < nfa->node_cnt; i++)
		if (nfa->nodes[i].lambda_cnt != 0) {
			isneed = 1;
			break;
		}

	if (!isneed)
		return 0;

	struct nfa	fa;

	size_t		*tmp = malloc(sizeof(size_t) * (nfa->node_cnt + 1)),
			tmp_cnt;

	nfa_alloc(&fa);

	fa.first_index = nfa->first_index;

	for (size_t i = 0; i < nfa->node_cnt; i++)
		nfa_add_node(&fa, NULL);

	for (size_t i = 0; i < nfa->node_cnt; i++) {
		fa.nodes[i].isfinal = nfa->nodes[i].isfinal;

		for (unsigned int j = 0; j < 256; j++) {
			for (size_t k = 0; k < nfa->nodes[i].trans_cnt[j]; k++) {
				nfa_lambda_reachable(tmp, &tmp_cnt,
						nfa->nodes[i].trans[j][k], nfa);

				for (size_t l = 0; l < tmp_cnt; l++)
					nfa_add_trans(&fa, i, (unsigned char) j,
						tmp[l]);
			}
		}
	}

	if (nfa->nodes[nfa->first_index].lambda_cnt != 0) {
		size_t	real_first,
			old_first = nfa->first_index;
		nfa_add_node(&fa, &real_first);
		nfa_lambda_reachable(tmp, &tmp_cnt, old_first, nfa);

		for (size_t i = 0; i < tmp_cnt; i++) {
			for (unsigned int j = 0; j < 256; j++) {
				for (size_t k = 0; k < fa.nodes[tmp[i]].trans_cnt[j]; k++) {
					nfa_add_trans(&fa, real_first, (unsigned char) j,
						      fa.nodes[tmp[i]].trans[j][k]);
				}
			}
		}
		fa.first_index = real_first;
	}

	fa.comment = nfa->comment;
	fa.comment_size = nfa->comment_size;
	nfa->comment = NULL;
	nfa->comment_size = 0;

	nfa_free(nfa);

	*nfa = fa;

	free(tmp);

	return 0;
}

int nfa_lambda_reachable(size_t *dst, size_t *cnt, size_t from, struct nfa *src)
{
	*cnt = 0;
	dst[(*cnt)++] = from;
	for (size_t i = 0; i < *cnt; i++)
		for (size_t j = 0; j < src->nodes[dst[i]].lambda_cnt; j++) {
			int	isnew = 1;
			for (size_t k = 0; k < *cnt; k++)
				if (dst[k] == src->nodes[dst[i]].lambda_trans[j]) {
					isnew = 0;
					break;
				}
			if (isnew)
				dst[(*cnt)++] = src->nodes[dst[i]].lambda_trans[j];
		}

	return 0;
}

int nfa_trans_reachable(size_t *dst, size_t *cnt, size_t from, struct nfa *src)
{
	*cnt = 0;
	dst[(*cnt)++] = from;
	for (size_t i = 0; i < *cnt; i++) {
		for (unsigned int j = 0; j < 256; j++) {
		for (size_t k = 0; k < src->nodes[dst[i]].trans_cnt[j]; k++) {
			int	isnew = 1;
			for (size_t l = 0; l < *cnt; l++)
				if (dst[l] == src->nodes[dst[i]].trans[j][k]) {
					isnew = 0;
					break;
				}
			if (isnew)
				dst[(*cnt)++] = src->nodes[dst[i]].trans[j][k];
		}
		}
	}

	return 0;
}

#define GET_BIT(a,b) (((a)[(b) / 8] >> ((b) % 8)) & 1)
#define SET_BIT(a,b) (a)[(b) / 8] |= (1 << ((b) % 8))

void print_char(unsigned char a)
{
	if (isprint(a)) {
		if (a == '"' || a == '-' || a == '\\')
			printf("\\");
		printf("%c", a);
	} else {
		printf("0x%02X", a);
	}
}


void print_charclass(char cc[256 / 8])
{
	int	total_cnt = 0, last = 0;
	for (int i = 0; i < 256; i++)
		if (GET_BIT(cc, i)) {
			total_cnt++;
			last = i;
		}

	if (total_cnt == 1) {
		print_char(last);
		return;
	}
	if (total_cnt == 256) {
		printf("<any>");
		return;
	}

	printf("[");
	if (total_cnt > 256 / 2) {
		printf("^");
		for (int i = 0; i < 256 / 8; i++)
			cc[i] = ~cc[i];
	}
	unsigned int	bb, tb;

	for (bb = 0; bb < 256;) {
		if (!GET_BIT(cc, bb)) {
			bb++;
		} else {
			for (tb = bb; tb < 256 && GET_BIT(cc, tb); tb++);
			tb--;


			if (bb == tb) {
				print_char(bb);
			} else if (tb - bb == 1) {
				print_char(bb);
				print_char(tb);
			} else {
				print_char(bb);
				printf("-");

				if (tb == ']' || tb == '"' || tb == '\\') {
					printf("\\%c", tb);
				} else if (isprint(tb)){
					printf("%c", tb);
				} else {
					print_char(tb);
				}
			}
			bb = tb + 1;
		}
	}
	printf("]");
}

void nfa_print(struct nfa *dst)
{
	struct nfa_node	*node;
	size_t	i;
	printf("#NFA: cnt[%zu], mem_size[%zu], first[%zu]\n", dst->node_cnt,
							      dst->node_mem_size,
							      dst->first_index);
	for (i = 0, node = dst->nodes; i < dst->node_cnt; i++, node++) {
		printf("{node [shape = %s, %s]; \"%zu\";}\n",
		       (node->isfinal ? "doublecircle" : "circle"),
		       (i == dst->first_index ? "style=bold" : ""),
			node->index);

		if (node->self_closed)
			printf("#deadend\n");
	}

	char	*marked = malloc(dst->node_cnt);

	for (i = 0, node = dst->nodes; i < dst->node_cnt; i++, node++) {
		printf("#node[%4zu]\n", node->index);
		if (node->lambda_cnt > 0) {
/*			printf("λ-transitions:");*/
		for (size_t j = 0; j < node->lambda_cnt; j++)
			printf("	\"%zu\"	-> \"%zu\"	[label = \"λ\"];\n", node->index, node->lambda_trans[j]);
		}

		char	marks[256/8];
		memset(marked, 0x00, dst->node_cnt);
		for (size_t j = 0; j < 256; j++) {
			for (size_t k = 0; k < node->trans_cnt[j]; k++) {
				size_t	to = node->trans[j][k];
				if (marked[to] == 0) {
					marked[to] = 1;
					memset(marks, 0x00, 256 / 8);
					for (int jj = j; jj < 256; jj++) {
						for (size_t kk = 0; kk < node->trans_cnt[jj]; kk++)
							if (node->trans[jj][kk] == to) {
								SET_BIT(marks, jj);
								break;
							}
					}
					printf("	\"%zu\"	-> \"%zu\"	[label = \"", i, to);
					print_charclass(marks);
					printf("\"];\n");
				}
			}
		}
	}
	free(marked);
}

int nfa_node_alloc(struct nfa_node *dst, size_t index)
{
	dst->index = index;
	dst->isfinal = 0;
	dst->self_closed = 0;
	dst->prefinal = 0;
	dst->lambda_trans = NULL;
	dst->lambda_cnt = 0;
	for (size_t i = 0; i < ARRAY_SIZE(dst->trans); i++) {
		dst->trans[i] = NULL;
		dst->trans_cnt[i] = 0;
	}
	return 0;
}

void nfa_node_free(struct nfa_node *dst)
{
	if (dst != NULL) {
		free(dst->lambda_trans);
		for (size_t i = 0; i < ARRAY_SIZE(dst->trans); i++)
			free(dst->trans[i]);
	}
}

int nfa_node_add_lambda_trans(struct nfa_node *dst, size_t to)
{
	void *tmp = dst->lambda_trans;

	for (size_t i = 0; i < dst->lambda_cnt; i++)
		if (dst->lambda_trans[i] == to)
			return 0;

	dst->lambda_trans = malloc(sizeof((dst->lambda_trans)[0])
				   * (dst->lambda_cnt + 1));

	memcpy(dst->lambda_trans, tmp, sizeof((dst->lambda_trans)[0])
				       * dst->lambda_cnt);

	free(tmp);

	dst->lambda_trans[dst->lambda_cnt++] = to;

	return 0;
}

int nfa_node_add_trans(struct nfa_node *dst, unsigned char mark, size_t to)
{
	void *tmp = dst->trans[mark];

	for (size_t i = 0; i < dst->trans_cnt[mark]; i++)
		if (dst->trans[mark][i] == to)
			return 0;

	dst->trans[mark] = malloc(sizeof((dst->trans)[0])
				  * (dst->trans_cnt[mark] + 1));

	memcpy(dst->trans[mark], tmp, sizeof((dst->trans)[0])
				      * dst->trans_cnt[mark]);

	free(tmp);

	dst->trans[mark][dst->trans_cnt[mark]++] = to;

	return 0;
}

int nfa_node_remove_trans(struct nfa_node *dst, unsigned char mark, size_t to)
{
	for (size_t i = 0; i < dst->trans_cnt[mark]; i++)
		if (dst->trans[mark][i] == to) {
			memmove(dst->trans[mark] + i, dst->trans[mark] + i + 1,
				(dst->trans_cnt[mark] - 1 - i) * sizeof(size_t));
			dst->trans_cnt[mark]--;
			return 0;
		}

	return -1;
}

int nfa_node_remove_trans_all(struct nfa_node *node)
{
	node->lambda_cnt = 0;
	free(node->lambda_trans);
	node->lambda_trans = NULL;

	for (int a = 0; a < 256; a++) {
		node->trans_cnt[a] = 0;
		free(node->trans[a]);
		node->trans[a] = NULL;
	}

	return 0;
}
