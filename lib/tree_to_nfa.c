/*
 * Conversion of regexp_tree to nfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include "tree_to_nfa.h"

int regexp_node_to_subnfa(struct nfa *, size_t, size_t, struct regexp_node *);

/*
 * @1 - pointer to EXISTING INITIALIZED nfa
 * @2 - pointer to regexp_tree
 * value: 0 if ok
 */
int convert_tree_to_lambdanfa(struct nfa *dst, struct regexp_tree *src)
{
	size_t	first, last;

	nfa_add_node(dst, &first);
	nfa_add_node(dst, &last);

	dst->first_index = first;
	dst->nodes[last].isfinal = 1;

	regexp_node_to_subnfa(dst, first, last, &(src->root));

	dst->comment_size = src->comment_size;
	dst->comment = malloc(dst->comment_size);
	memcpy(dst->comment, src->comment, dst->comment_size);

	return 0;
}

int regexp_node_to_subnfa_norepeat(struct nfa *dst, size_t from, size_t to,
				      struct regexp_node *src);

int regexp_node_to_subnfa(struct nfa *dst, size_t from, size_t to,
			      struct regexp_node *src)
{
	size_t	index1, index2;
	int	res = 0;

/*	if (src->repeat.min == 0)
		nfa_add_lambda_trans(dst, from, to);*/
/* ADD check of src->repeat values */

	for (int i = 0; i < src->repeat.min; i++) {
		nfa_add_node(dst, &index1);
		regexp_node_to_subnfa_norepeat(dst, from, index1, src);
		from = index1;
	}

	nfa_add_lambda_trans(dst, from, to);

	if (src->repeat.max == -1) {
		nfa_add_node(dst, &index1);
		nfa_add_node(dst, &index2);

		nfa_add_lambda_trans(dst, from, index1);
		nfa_add_lambda_trans(dst, index2, to);
		nfa_add_lambda_trans(dst, index2, index1);

		regexp_node_to_subnfa_norepeat(dst, index1, index2, src);
	} else {
		for (int i = 0; i < src->repeat.max - src->repeat.min; i++) {
			nfa_add_node(dst, &index1);
			regexp_node_to_subnfa_norepeat(dst, from, index1, src);
			nfa_add_lambda_trans(dst, index1, to);
			from = index1;
		}
	}


//	res = regexp_node_to_subnfa_norepeat(dst, from, to, src);
	return res;
}

int regexp_node_to_subnfa_norepeat(struct nfa *dst, size_t from, size_t to,
				      struct regexp_node *src)
{
	size_t	index1, index2;

/*	if (src->repeat.min == 0)
		nfa_add_lambda_trans(dst, from, to);

	if (src->repeat.max == -1) {
		nfa_add_node(dst, &index1);
		nfa_add_node(dst, &index2);
		nfa_add_lambda_trans(dst, from, index1);
		nfa_add_lambda_trans(dst, index2, to);
		nfa_add_lambda_trans(dst, index2, index1);
		from = index1;
		to = index2;
	}*/

	switch (src->type) {
	case RE_CHAR:
		nfa_add_trans(dst, from, src->data.c_val, to);
		break;
	case RE_CHARCLASS:
		for (unsigned int i = 0; i < 256; i++)
			if (GET_BIT(src->data.cc_data.data, i) != src->data.cc_data.inverse)
				nfa_add_trans(dst, from, (unsigned char) i, to);
		break;
	case RE_CONCAT:
		if (src->data.childs.cnt == 0) {
			nfa_add_lambda_trans(dst, from, to);
			return 0;
		}

		index1 = from;
		for (int i = 0; i < src->data.childs.cnt; i++) {
			if (i == src->data.childs.cnt - 1)
				index2 = to;
			else
				nfa_add_node(dst, &index2);
			regexp_node_to_subnfa(dst, index1, index2,
					      src->data.childs.ptr[i]);
			index1 = index2;
		}

		break;
	case RE_UNION:
		if (src->data.childs.cnt == 0) {
			nfa_add_lambda_trans(dst, from, to);
			return 0;
		}

		for (int i = 0; i < src->data.childs.cnt; i++) {
			nfa_add_node(dst, &index1);
			nfa_add_lambda_trans(dst, from, index1);
			nfa_add_node(dst, &index2);
			nfa_add_lambda_trans(dst, index2, to);

			regexp_node_to_subnfa(dst, index1, index2,
					      src->data.childs.ptr[i]);
		}

		break;
	case RE_EMPTY:
		nfa_add_lambda_trans(dst, from, to);
		break;
	default:
		return -1;
	};

	return 0;
}
