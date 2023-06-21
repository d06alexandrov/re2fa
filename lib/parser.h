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
#include <stdbool.h>

/**
 * Get a bit value from an array by it's index
 *
 * \param a	char* array
 * \param b	bit's index
 */
#define GET_BIT(a,b) ((((char*)(a))[(b) / 8] >> ((b) % 8)) & 1)

/**
 * Set a bit value to 1 from an array by it's index
 *
 * \param a	char* array
 * \param b	bit's index
 */
#define SET_BIT(a,b) ((char*)(a))[(b) / 8] |= (1 << ((b) % 8))

/**
 * Set a bit value to 0 from an array by it's index
 *
 * \param a	char* array
 * \param b	bit's index
 */
#define RST_BIT(a,b) ((char*)(a))[(b) / 8] &= ~(1 << ((b) % 8))

/* binary `or' for `cnt' bits from `off1' to `off2' */
/**
 * Apply binary 'or' operation to two continous bit's group
 * and save it to the first group.
 *
 * \param a	char* array
 * \param off1	offset of the first (destination) group
 * \param off2	offset of the second (source) group
 * \param cnt	width of bit's group
 */
#define OR_BIT_GRP(a,off1,off2,cnt)		\
	do {								\
		for (int _ob_cnt = 0; _ob_cnt < (cnt); _ob_cnt++)	\
			if (GET_BIT((a), (off2) + _ob_cnt))		\
				SET_BIT((a), (off1) + _ob_cnt);		\
	} while (0)

/**
 * structure that represents regular expression's character class
 */
struct re_charclass {
	/**
	 * bitmask of characters in the class
	 */
	unsigned char data[256 / 8];

	/**
	 * inverted mask flag
	 */
	bool inverse;
};

/**
 * node's type
 */
enum re_node_type {
	/**
	 * deprecated
	 */
	RE_SPECIAL	= 0x01,
	/**
	 * one character 'a'
	 */
	RE_CHAR		= 0x02,
	/**
	 * character class '[abc]'
	 */
	RE_CHARCLASS	= 0x04,
	/**
	 * concatenation of nodes 'abc'
	 */
	RE_CONCAT	= 0x08,
	/**
	 * union of nodes 'a|b|c'
	 */
	RE_UNION	= 0x10,
	/**
	 * empty node
	 */
	RE_EMPTY	= 0x20
};

/**
 * node of the regexp_tree
 */
struct regexp_node {
	/**
	 * node type
	 */
	enum re_node_type type;

	/**
	 * number of node repetition
	 */
	struct {
		int min, max;
	} repeat;

	/**
	 * node's data
	 */
	union {
		/**
		 * RE_CHAR value
		 */
		unsigned char c_val;
		/**
		 * RE_CHARCLASS set
		 */
		struct re_charclass cc_data;
		/**
		 * RE_CONCAT and RE_UNION child nodes
		 */
		struct {
			/**
			 * how many space was allocated for pointers
			 */
			int max_cnt;
			/**
			 * how many childs added
			 */
			int cnt;
			/**
			 * array of pointers to child nodes
			 */
			struct regexp_node **ptr;
		} childs;
	} data;

	/**
	 * pointer to the parent node
	 */
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
