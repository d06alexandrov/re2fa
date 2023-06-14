/*
 * Regular expressions parser.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef __PARSER_H
#define __PARSER_H

#include <stddef.h>

#define GET_BIT(a,b) (((a)[(b) / 8] >> ((b) % 8)) & 1)
#define SET_BIT(a,b) (a)[(b) / 8] |= (1 << ((b) % 8))
#define RST_BIT(a,b) (a)[(b) / 8] &= ~(1 << ((b) % 8))
/* binary `or' for `cnt' bits from `off2' to `off2' */
#define OR_BIT_GRP(a,off1,off2,cnt)		\
	do {								\
		for (int _ob_cnt = 0; _ob_cnt < (cnt); _ob_cnt++)	\
			if (GET_BIT((a), (off2) + _ob_cnt))		\
				SET_BIT((a), (off1) + _ob_cnt);		\
	} while (0)

struct re_charclass {
	int		inverse;
	unsigned char	data[256 / 8];
};

struct regexp_node {
	enum re_type {
		RE_SPECIAL	= 0x01,/* temporary for ^ and $ */
				       /* deprecated! */
		RE_CHAR		= 0x02,
		RE_CHARCLASS	= 0x04,
		RE_CONCAT	= 0x08,
		RE_UNION	= 0x10,
		RE_EMPTY	= 0x20
	} type;

	struct {
		int min, max;
	} repeat;

	union {
		unsigned char		c_val;
		struct re_charclass	cc_data;
		struct {
			int			max_cnt;
			int			cnt;
			struct regexp_node	**ptr;
		} childs;
	} data;

	struct regexp_node	*parent;
};

struct regexp_tree {
	char			*comment;
	size_t			comment_size;
	struct regexp_node	root;
};

/*
 * parse string with regexp to tree (regexp_node's)
 * @1	regexp string
 * @2	pointer to error, if occured
 * result:
 *	  NULL if error
 *	  otherwise pointer to regexp tree
 */
void *regexp_to_tree(const char *, const char **);

/*
 * frees allocated memory
 */
void regexp_tree_free(struct regexp_tree *);

/*
 * prints regexp_tree for debug purposes
 */
void regexp_tree_print(struct regexp_tree *);

#endif
