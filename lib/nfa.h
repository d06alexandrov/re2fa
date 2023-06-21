/*
 * Declaration of nondeterministic finite automaton.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

/**
 * @addtogroup nfa nfa
 * @{
 */

#ifndef REFA_NFA_H
#define REFA_NFA_H

#include <stddef.h>
#include <stdbool.h>

/**
 * structure that represents Non-deterministic Finite-state Automaton's state
 */
struct nfa_node {
	/**
	 * index of this node (state)
	 */
	size_t index;

	/**
	 * is this node is an accepting (final) state
	 */
	bool isfinal;

	/**
	 * is this node is self closed state (all transitions go to itself)
	 */
	bool self_closed;

	/**
	 * are all transitions from this node go the final states
	 */
	bool prefinal;

	/**
	 * list of lambda-transitions
	 */
	size_t *lambda_trans;

	/**
	 * total number of lambda transitions
	 */
	size_t lambda_cnt;

	/**
	 * regular transitions
	 */
	size_t *trans[256];

	/**
	 * total number of regular transitions
	 */
	size_t trans_cnt[256];
};

/**
 * structure that represents Non-deterministic Finite-state Automaton's (NFA)
 */
struct nfa {
	/**
	 * char string with arbitrary information
	 * using mainly for storing original regular expression
	 */
	char *comment; /* char string with any info */

	/**
	 * size of allocated for the comment memory
	 */
	size_t comment_size;

	/**
	 * total number of nfa states
	 */
	size_t node_cnt;

	/**
	 * number of allocated states (greater or equal to the node_cnt)
	 */
	size_t node_mem_size;

	/**
	 * array that holds all nfa states (nodes)
	 */
	struct nfa_node	*nodes;

	/**
	 * index of the first (initial) state
	 */
	size_t first_index;
};

/**
 * Initialization of NFA structure.
 *
 * Simple initialization of the NFA object.
 *
 * @param nfa	pointer to the nfa structure
 * @return	0 on success
 */
int nfa_alloc(struct nfa *nfa);

/**
 * Deinitialization of NFA structure.
 *
 * Just freeing memory allocated for the NFA inner fields.
 *
 * @param nfa	pointer to the nfa structure
 */
void nfa_free(struct nfa *nfa);

/**
 * Rebuild of NFA structure.
 *
 * Removing unreachable states and lambda-transitions.
 *
 * @param nfa	pointer to the nfa structure
 * @return	0 on success
 */
int nfa_rebuild(struct nfa *nfa);

/**
 * Join two NFA.
 *
 * Joins two NFA - dst and src (dst|src) and saves the result to dst.
 *
 * @param dst	pointer to the nfa structure where result will be stored
 * @param src	pointer to the nfa structure that will be joined to the dst
 * @return	0 on success
 */
int nfa_join(struct nfa *dst, struct nfa *src);

/**
 * Add new node (state) to the NFA.
 *
 * Adds new empty state to the NFA and returns the newly created state's index.
 *
 * @param nfa	pointer to the nfa structure
 * @param index	place where new index will be saved if not NULL
 * @return	0 on success
 */
int nfa_add_node(struct nfa *nfa, size_t *index);

/**
 * Add multiple new states to the NFA.
 *
 * Adds multiple new empty states to the NFA and returns index of the first
 * newly created state.
 *
 * @param nfa	pointer to the nfa structure
 * @param cnt	how many states must be added
 * @param index	place where new index will be saved if not NULL
 * @return	0 on success
 */
int nfa_add_node_n(struct nfa *nfa, size_t cnt, size_t *index);

/**
 * Check if NFA's state is an accepting (final) state.
 *
 * Checks if the state with provided index is marked as final.
 *
 * @param nfa	pointer to the nfa structure
 * @param state	index of the NFA's state
 * @return	0 if the state with provided index is final
 */
int nfa_state_is_final(struct nfa *nfa, size_t state);

/**
 * Mark or unmark the NFA's state as a final state.
 *
 * Marks the state with provided index as final or remove this mark.
 *
 * @param nfa	pointer to the nfa structure
 * @param state	index of the NFA's state
 * @param final	0 to clear 'final' mark and 1 to set
 * @return	0 on success
 */
int nfa_state_set_final(struct nfa *nfa, size_t state, int final);

/**
 * Add lambda-transition to NFA.
 *
 * Add lambda-transition between two states.
 *
 * @param nfa	pointer to the nfa structure where transition will be added
 * @param from	left state's index
 * @param to	right state's index
 * @return	0 on success
 */
int nfa_add_lambda_trans(struct nfa *nfa, size_t from, size_t to);

/**
 * Add transition to NFA.
 *
 * Adds transition between two states.
 *
 * @param nfa	pointer to the nfa structure where transition will be added
 * @param from	left state's index
 * @param mark	label of transition
 * @param to	right state's index
 * @return	0 on success
 */
int nfa_add_trans(struct nfa *nfa, size_t from, unsigned char mark, size_t to);

/**
 * Remove transition from NFA.
 *
 * Removes transition between two states.
 *
 * @param nfa	pointer to the nfa from where transitions will be removed
 * @param from	left state's index
 * @param mark	label of transition
 * @param to	right state's index
 * @return	0 on success
 */
int nfa_remove_trans(struct nfa *nfa, size_t from, unsigned char mark, size_t to);

/**
 * Remove all transitions from specific NFA's state.
 *
 * Removes all transitions from the state.
 *
 * @param nfa	pointer to the nfa from where transitions will be removed
 * @param from	left state's index
 * @return	0 on success
 */
int nfa_remove_trans_all(struct nfa *nfa, size_t from);

/**
 * Copy transitions from one NFA's state to another.
 *
 * Copies all outgoing transitions from one state to another.
 *
 * @param nfa	pointer to the nfa with corresponding states
 * @param to	state where transitions will be copied
 * @param from	state which transitions will be copied
 * @return	0 on success
 */
int nfa_copy_trans(struct nfa *nfa, size_t to, size_t from);

/**
 * Remove all lambda-transitions by rebuilding NFA.
 *
 * Removes all lambda-transitions from NFA but keeps accepted regular language.
 *
 * @param nfa	pointer to the nfa from where lambda-transitions will be removed
 * @return	0 on success
 */
int nfa_remove_lambda(struct nfa *nfa);

#endif /** REFA_NFA_H @} */
