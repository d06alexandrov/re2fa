/*
 * Regular expressions parser.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

/**
 * @addtogroup expression_parser expression parser
 * @{
 */

#ifndef REFA_PARSER_H
#define REFA_PARSER_H

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

/**
 * special tree-like structure to store parsed regular expression
 */
struct regexp_tree {
	/**
	 * char string with arbitrary information
	 * using mainly for storing original regular expression
	 */
	char *comment;

	/**
	 * size of allocated for the comment memory
	 */
	size_t comment_size;

	/**
	 * root of the regular expression's tree
	 */
	struct regexp_node root;
};

/**
 * Parse regular expression into tree.
 *
 * Parses regular expression and creates regexp tree if possible.
 *
 * @param regexp	regular expression to parse
 * @param err		reserved parameter, must be NULL
 * @return		pointer to the regexp tree on success, otherwise NULL
 */
struct regexp_tree *regexp_to_tree(const char *regexp, const char **err);

/**
 * Free regexp tree.
 *
 * Frees regexp tree structure.
 *
 * @param re_tree	pointer to the regexp tree
 */
void regexp_tree_free(struct regexp_tree *re_tree);

/**
 * Print regexp tree.
 *
 * Prints regexp tree structure for debug purpose.
 *
 * @param re_tree	pointer to the regexp tree
 */
void regexp_tree_print(struct regexp_tree *re_tree);

#endif /** REFA_PARSER_H @} */
