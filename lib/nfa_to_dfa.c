/*
 * Conversion of nfa to dfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nfa_to_dfa.h"

/**
 * @brief One node with queue elements.
 */
struct ptr_queue_node {
	/**
	 * @brief Pointer to the next node.
	 */
	struct ptr_queue_node *next_node;

	/**
	 * @brief Pointer to the first allocated element.
	 */
	void **first;

	/**
	 * @brief Pointer to the position for the new element.
	 */
	void **last;

	/**
	 * @brief Pointer to the end of memory buffer.
	 */
	void **end;

	/**
	 * @brief Pointers buffer.
	 */
	void *mem[];
};

/**
 * @brief Queue of pointers.
 */
struct ptr_queue {
	/**
	 * @brief Size of one node of a queue.
	 */
	unsigned long pagesize;

	/**
	 * @brief Number of elements in one node of a queue.
	 */
	size_t node_elements;

	/**
	 * @brief Node with first element.
	 */
	struct ptr_queue_node *first_node;

	/**
	 * @brief Node with last element.
	 */
	struct ptr_queue_node *last_node;

	/**
	 * @brief Reserved node.
	 */
	struct ptr_queue_node *reserved;
};

/**
 * @brief Initialize queue of pointers.
 *
 * Initialize queue structure.
 *
 * @param q	pointer to the queue
 * @return	0 on success
 */
static void ptr_queue_init(struct ptr_queue *q)
{
	q->pagesize = (unsigned long)sysconf(_SC_PAGESIZE);
	q->node_elements = (q->pagesize - sizeof(struct ptr_queue_node))
			   / sizeof(void *);

	q->first_node = NULL;
	q->last_node = NULL;
	q->reserved = NULL;
}

/**
 * @brief Free queue of pointers.
 *
 * Free memory allocated for queue of pointers.
 *
 * @param q	pointer to the queue
 */
static void ptr_queue_deinit(struct ptr_queue *q)
{
	struct ptr_queue_node *next_node;
	struct ptr_queue_node *cur_node;

	next_node = q->first_node;

	while (next_node != NULL) {
		cur_node = next_node;
		next_node = cur_node->next_node;

		free(cur_node);
	}

	free(q->reserved);
}

/**
 * @brief Add new element to the queue's end.
 *
 * @param q	pointer to the queue
 * @param ptr	element (pointer) to add to the queue
 * @return	0 on success
 */
static int ptr_queue_push(struct ptr_queue *q, void *ptr)
{
	struct ptr_queue_node *node;
	struct ptr_queue_node *new_node;

	node = q->last_node;

	if (node == NULL || node->last == node->end) {
		if (q->reserved != NULL) {
			new_node = q->reserved;
			q->reserved = NULL;
		} else {
			new_node = aligned_alloc(q->pagesize, q->pagesize);
		}

		if (new_node != NULL) {
			new_node->next_node = NULL;
			new_node->first = new_node->mem;
			new_node->last = new_node->first;
			new_node->end = new_node->first + q->node_elements;

			if (node == NULL) {
				q->first_node = new_node;
			} else {
				node->next_node = new_node;
			}

			q->last_node = new_node;
		}

		node = new_node;
	}

	if (node != NULL) {
		*node->last = ptr;
		node->last++;
	}

	return node == NULL;
}

/**
 * @brief Get first element from the queue.
 *
 * @param q	pointer to the queue
 * @return	pointer from queue
 *		NULL if queue is empty
 */
static void *ptr_queue_pop(struct ptr_queue *q)
{
	struct ptr_queue_node *node;
	void *res = NULL;

	node = q->first_node;

	if (node != NULL && node->first != node->last) {
		res = *node->first;
		node->first++;

		if (node->first == node->end) {
 			q->first_node = node->next_node;

			if (node == q->last_node) {
				q->last_node = NULL;
			}

			if (q->reserved == NULL) {
				q->reserved = node;
			} else {
				free(node);
			}
		}
	}

	return res;
}

/**
 * @brief Pair of corresponding DFA and NFA states.
 */
struct nfa_dfa_pair {
	/**
	 * @brief Index of the DFA state.
	 */
	size_t dfa_state;

	/**
	 * @brief Number of NFA states in the NFA states set.
	 */
	size_t nfa_count;

	/**
	 * @brief Capacity of the NFA states set.
	 */
	size_t nfa_count_reserved;

	/**
	 * @brief NFA states set.
	 */
	size_t nfa_states[];
};

/**
 * @brief Increment of NFA states set in nfa_dfa_pair.
 */
#define NFA_DFA_PAIR_STEP 8

/**
 * @brief Allocate memory for nfa dfa pair.
 *
 * @return	new nfa_dfa_pair on success, NULL otherwise
 */
static struct nfa_dfa_pair *nfa_dfa_pair_alloc()
{
	struct nfa_dfa_pair *pair;

	pair = malloc(NFA_DFA_PAIR_STEP * sizeof(size_t));

	if (pair != NULL) {
		pair->nfa_count_reserved = NFA_DFA_PAIR_STEP
					   - sizeof(struct nfa_dfa_pair) / sizeof(size_t);
		pair->nfa_count = 0;
		pair->dfa_state = 0;
	}

	return pair;
}

/**
 * @brief Free nfa dfa pair.
 *
 * Free memory allocated for the (nfa;dfa) pair.
 * @todo Put freed memory into some temporary place, and alloc from it.
 *
 * @param pair	pointer to the pait that has to be freed
 */
static void nfa_dfa_pair_free(struct nfa_dfa_pair *pair)
{
	free(pair);
}

/**
 * @brief Add NFA state to the nfa dfa pair.
 *
 * Add NFA state to the pair if it is not there already.
 *
 * @param pair		recipient pair
 * @param element	NFA state index to add.
 * @return		0 on success
 */
static int nfa_dfa_pair_add(struct nfa_dfa_pair **pair, size_t element)
{
	size_t new_pos;
	size_t left;
	size_t right;
	size_t middle;
	size_t new_mem_size;
	struct nfa_dfa_pair *mem;

	if ((*pair)->nfa_count == 0) {
		new_pos = 0;
	} else {
		left = 0;
		right = (*pair)->nfa_count - 1;

		while (left < right) {
			middle = (left + right) / 2;

			if ((*pair)->nfa_states[middle] < element) {
				left = middle + 1;
			} else {
				right = middle;
			}
		}

		if ((*pair)->nfa_states[left] != element) {
			if ((*pair)->nfa_states[left] < element) {
				new_pos = left + 1;
			} else {
				new_pos = left;
			}
		} else {
			goto found;
		}

	}

	if ((*pair)->nfa_count_reserved == (*pair)->nfa_count) {
		new_mem_size = (*pair)->nfa_count_reserved +
			       NFA_DFA_PAIR_STEP;

		mem = malloc(sizeof(struct nfa_dfa_pair) +
			     sizeof(size_t) * new_mem_size);

		if (mem == NULL) {
			return -1;
		}

		memcpy(mem, *pair, sizeof(struct nfa_dfa_pair));
		memcpy(mem->nfa_states,
		       (*pair)->nfa_states,
		       sizeof(size_t) * new_pos);
		memcpy(mem->nfa_states + new_pos + 1,
		       (*pair)->nfa_states + new_pos,
		       sizeof(size_t) * ((*pair)->nfa_count - new_pos));

		free(*pair);

		*pair = mem;
		(*pair)->nfa_count_reserved = new_mem_size;
	} else {
		memmove((*pair)->nfa_states + new_pos + 1,
			(*pair)->nfa_states + new_pos,
			sizeof(size_t) * ((*pair)->nfa_count - new_pos));
	}

	(*pair)->nfa_states[new_pos] = element;
	(*pair)->nfa_count++;

found:
	return 0;
}

/**
 * @brief Calculate next set of NFA states by it's previous set and
 * transition mark.
 *
 * @param nfa		original NFA
 * @param current	nfa_dfa_pair where the current set is stored
 * @param mark		transition mark
 * @param final		is the next set have final states
 * @return		next NFA states' set or NULL on failure
 */
static struct nfa_dfa_pair *nfa_dfa_pair_next_state(
				const struct nfa *nfa,
				const struct nfa_dfa_pair *current,
				unsigned char mark,
				bool *final)
{
	struct nfa_dfa_pair *next = NULL;
	bool error = false;
	*final = false;

	if ((next = nfa_dfa_pair_alloc()) == NULL) {
		error = true;
	}

	for (size_t state = 0; state < current->nfa_count && !error; state++) {
		size_t nfa_index;
		size_t trans_cnt;
		size_t *nfa_trans;

		nfa_index = current->nfa_states[state];

		trans_cnt = nfa_get_trans(nfa, nfa_index,
					  mark, &nfa_trans);

		for (size_t trans = 0; trans < trans_cnt && !error; trans++) {
			size_t nfa_index_next;

			nfa_index_next = nfa_trans[trans];

			if (!*final) {
				*final = nfa_state_is_final(nfa, nfa_index_next);
			}

			error = nfa_dfa_pair_add(&next, nfa_index_next) != 0;
		}
	}

	if (error && next != NULL) {
		nfa_dfa_pair_free(next);
		next = NULL;
	}

	return next;
}

/**
 * @brief Compare two nfa dfa pairs.
 *
 * Compare lexicographically NFA states sets.
 *
 * @param p1	left pair to compare
 * @param p2	right pair to compare
 * @return	0 if \p p1 equals to \p p2
 *		-1 if \p p1 less than \p p2
 *		1 if \p p1 greater than \p p2
 */
static int compare_nfa_dfa_pair(struct nfa_dfa_pair *p1, struct nfa_dfa_pair *p2)
{
	int cmp = 0;
	size_t min_state_count;
	bool compared = false;

	if (p1->nfa_count < p2->nfa_count) {
		min_state_count = p1->nfa_count;
	} else {
		min_state_count = p2->nfa_count;
	}

	for (size_t i = 0; i < min_state_count && !compared; i++) {
		if (p1->nfa_states[i] != p2->nfa_states[i]) {
			if (p1->nfa_states[i] < p2->nfa_states[i]) {
				cmp = -1;
			} else {
				cmp = 1;
			}

			compared = true;
		}
	}

	if (!compared) {
		if (p1->nfa_count < p2->nfa_count) {
			cmp = -1;
		} else if (p1->nfa_count > p2->nfa_count) {
			cmp = 1;
		} else {
			cmp = 0;
		}
	}

	return cmp;
}

/**
 * @brief Red-black tree node for nfa dfa pairs.
 */
struct rb_node {
	/**
	 * @brief Pointer to the parent node.
	 */
	struct rb_node *parent;

	/**
	 * @brief Pointer to the left child node.
	 */
	struct rb_node *left;

	/**
	 * @brief Pointer to the right child node.
	 */
	struct rb_node *right;

	/**
	 * @brief Pointer to the nfa dfa pair.
	 */
	struct nfa_dfa_pair *pair;

	/**
	 * @brief Is a black color node.
	 */
	bool black;
};

/**
 * @brief Red-black tree node for nfa dfa pairs.
 */
struct rb_tree {
	/**
	 * @brief Root node.
	 */
	struct rb_node *root;

	/**
	 * @brief Nil node.
	 */
	struct rb_node nil;
};

/**
 * @brief Initialize red-black tree.
 *
 * @param tree	pointer to the tree structure
 */
static void rb_tree_init(struct rb_tree *tree)
{
	tree->nil = (struct rb_node){.parent = NULL, .left = NULL,
				     .right = NULL, .pair = NULL, true};
	tree->root = &tree->nil;
}

/**
 * @brief Deinitialize tree.
 *
 * @param tree	pointer to the tree structure
 */
static void rb_tree_deinit(struct rb_tree *tree)
{
	struct rb_node *iter;
	struct rb_node *parent;

	if (tree->root == &tree->nil) {
		return;
	}

	parent = &tree->nil;
	iter = tree->root;

	while (iter != &tree->nil) {
		while (iter->left != &tree->nil || iter->right != &tree->nil) {
			if (iter->left != &tree->nil) {
				iter = iter->left;
			} else {
				iter = iter->right;
			}
		}

		parent = iter->parent;

		if (parent->left == iter) {
			parent->left = &tree->nil;
		} else {
			parent->right = &tree->nil;
		}

		nfa_dfa_pair_free(iter->pair);

		free(iter);
		iter = parent;
	}
}

/**
 * @brief Left rotation in red-black tree.
 *
 * @verbatim

          |                     |
         node                 child
        /    \       =>      /     \
       a    child          node      c
           /     \        /    \
          b       c      a      b

   @endverbatim
 *
 * @param tree	pointer to the tree structure
 * @param node	node to be rotated
 */
static void rb_tree_left_rotate(struct rb_tree *tree, struct rb_node *node)
{
	struct rb_node *child;

	child = node->right;

	node->right = child->left;

	if (node->right != &tree->nil) {
		node->right->parent = node;
	}

	child->parent = node->parent;
	if (child->parent == &tree->nil) {
		tree->root = child;
	} else if (node == node->parent->left) {
		node->parent->left = child;
	} else {
		node->parent->right = child;
	}

	child->left = node;
	node->parent = child;
}

/**
 * @brief Left rotation in red-black tree.
 *
 * @verbatim

             |              |
            node           child
           /    \    =>   /     \
         child   c       a      node
        /     \                /    \
       a       b              b      c

   @endverbatim
 *
 * @param tree	pointer to the tree structure
 * @param node	node to be rotated
 */
static void rb_tree_right_rotate(struct rb_tree *tree, struct rb_node *node)
{
	struct rb_node *child;

	child = node->left;

	node->left = child->right;

	if (node->left != &tree->nil) {
		node->left->parent = node;
	}

	child->parent = node->parent;
	if (child->parent == &tree->nil) {
		tree->root = child;
	} else if (node == node->parent->left) {
		node->parent->left = child;
	} else {
		node->parent->right = child;
	}

	child->right = node;
	node->parent = child;
}

/**
 * @brief Fix red-black property after insertion.
 *
 * @param tree	pointer to the tree structure
 * @param node	node to be fixed
 */
static void rb_tree_fix(struct rb_tree *tree, struct rb_node *node_to_fix)
{
	struct rb_node *node_uncle;
	struct rb_node *node = node_to_fix;

	while (node->parent->black == false) {
		if (node->parent == node->parent->parent->left) {
			node_uncle = node->parent->parent->right;
			if (node_uncle->black) {
				if (node == node->parent->right) {
					node = node->parent;
					rb_tree_left_rotate(tree, node);
				}

				node->parent->black = true;
				node->parent->parent->black = false;
				rb_tree_right_rotate(tree, node->parent->parent);
			} else {
				node->parent->black = true;
				node_uncle->black = true;

				node_uncle->parent->black = false;
				node = node_uncle->parent;
			}
		} else {
			node_uncle = node->parent->parent->left;
			if (node_uncle->black) {
				if (node == node->parent->left) {
					node = node->parent;
					rb_tree_right_rotate(tree, node);
				}

				node->parent->black = true;
				node->parent->parent->black = false;
				rb_tree_left_rotate(tree, node->parent->parent);
			} else {
				node->parent->black = true;
				node_uncle->black = true;

				node_uncle->parent->black = false;
				node = node_uncle->parent;
			}
		}
	}

	tree->root->black = true;
}

/**
 * @brief Try to add node to the tree.
 *
 * @param tree	pointer to the tree structure
 * @param pair	pointer to the nfa dfa pair to be added
 * @return	0 if successfully added pair to the tree
 *		1 if equal pair is already in the tree
 *		-1 if error occured
 */
static int rb_tree_try_add(struct rb_tree *tree, struct nfa_dfa_pair *pair)
{
	struct rb_node *iter;
	struct rb_node *parent;
	struct rb_node *new_node;
	int cmp_res;

	iter = tree->root;
	parent = &tree->nil;

	while (iter != &tree->nil) {
		parent = iter;
		cmp_res = compare_nfa_dfa_pair(pair, iter->pair);

		if (cmp_res < 0) {
			iter = iter->left;
		} else if (cmp_res > 0) {
			iter = iter->right;
		} else {
			pair->dfa_state = iter->pair->dfa_state;
			return 1;
		}
	}

	new_node = malloc(sizeof(struct rb_node));
	if (new_node == NULL) {
		return -1;
	}

	new_node->pair = pair;
	new_node->parent = parent;
	new_node->left = &tree->nil;
	new_node->right = &tree->nil;
	new_node->black = false;

	if (parent == &tree->nil) {
		tree->root = new_node;
	} else {
		if (cmp_res < 0) {
			parent->left = new_node;
		} else {
			parent->right = new_node;
		}
	}

	rb_tree_fix(tree, new_node);

	return 0;
}

int convert_nfa_to_dfa(struct dfa *dst, const struct nfa *src)
{
	struct ptr_queue q;
	struct rb_tree t;
	const struct nfa_dfa_pair *pair;
	struct nfa_dfa_pair *next_pair = NULL;
	size_t nfa_index;
	size_t dfa_index;
	int rb_added;
	bool final;
	bool failure = false;

	nfa_index = nfa_get_initial_state(src);

	ptr_queue_init(&q);

	rb_tree_init(&t);

	if (dfa_add_state(dst, &dfa_index) != 0) {
		failure = true;
	} else if ((next_pair = nfa_dfa_pair_alloc()) == NULL) {
		failure = true;
	} else if (nfa_dfa_pair_add(&next_pair, nfa_index) != 0) {
		failure = true;
	} else if (rb_tree_try_add(&t, next_pair) != 0) {
		failure = true;
	} else {
		next_pair->dfa_state = dfa_index;
	}

	if (failure) {
		nfa_dfa_pair_free(next_pair);
	}

	pair = next_pair;

	do {
		for (unsigned int i = 0; i < 256 && !failure; i++) {
			next_pair = nfa_dfa_pair_next_state(src, pair,
							    (unsigned char)i,
							    &final);
			if (next_pair == NULL) {
				failure = true;
				break;
			}

			rb_added = rb_tree_try_add(&t, next_pair);

			if (rb_added == 0) {
				if (dfa_add_state(dst, &dfa_index) != 0) {
					failure = true;
				} else {
					dfa_state_set_final(dst, dfa_index,
							    final);
					next_pair->dfa_state = dfa_index;
					failure = ptr_queue_push(&q, next_pair) != 0;
				}
			} else if (rb_added == 1) {
				dfa_index = next_pair->dfa_state;
				nfa_dfa_pair_free(next_pair);
			} else {
				nfa_dfa_pair_free(next_pair);
				failure = true;
			}

			if (!failure &&
			    dfa_add_trans(dst, pair->dfa_state,
					  (unsigned char)i, dfa_index) != 0) {
				failure = true;
			}
		}
	} while ((pair = ptr_queue_pop(&q)) != NULL && !failure);

	/*
	 * @todo Add not so fragile interface for setting up comment property.
	 */
	if (!failure) {
		dst->comment_size = src->comment_size;
		dst->comment = realloc(dst->comment, dst->comment_size);
		memcpy(dst->comment, src->comment, dst->comment_size);
	}

	rb_tree_deinit(&t);
	ptr_queue_deinit(&q);

	return !failure ? 0 : 1;
}
