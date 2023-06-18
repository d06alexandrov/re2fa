/*
 * Declaration of deterministic finite automaton.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

/**
 * @addtogroup dfa dfa
 * @{
 */

#ifndef REFA_DFA_H
#define REFA_DFA_H

#include <stdint.h>
#include <stddef.h>

#define DFA_FLAG_FINAL		(0x01)
#define DFA_FLAG_DEADEND	(0x02)

/**
 * structure that represents Deterministic Finite-state Automaton (DFA)
 */
struct dfa {
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
	 * total number of DFA states
	 */
	size_t state_cnt;

	/**
	 * number of allocated states (greater or equal to the state_cnt)
	 */
	size_t state_malloc_cnt;

	/**
	 * bits per state - how many bits are used to represent one state number
	 * always is a power of 2
	 */
	uint8_t bps; /* bits per state index */

	/**
	 * size of one row of the transition table
	 * equals to 256 * bps
	 */
	size_t state_size;

	/**
	 * maximum size of dfa
	 * equals to 0xFFFFFFFFFFFFFFFF
	 */
	uint64_t state_max_cnt;

	/**
	 * array that holds all transitions between states
	 */
	void *trans;

	/**
	 * array that holds information about states' types such as 'FINAL'
	 */
	uint8_t *flags;

	/**
	 * index of the first (initial) state
	 */
	size_t first_index;
};

/**
 * Initialization of DFA structure.
 *
 * Simple initialization of the DFA object with 64 bit states' indexes.
 *
 * @param dfa	pointer to the dfa structure
 * @return	0 on success
 */
int dfa_alloc(struct dfa *dfa);

/**
 * Initialization of DFA structure.
 *
 * Simple initialization of the DFA object but with explicitly choosen
 * maximum state's index
 *
 * @param dfa		pointer to the dfa structure
 * @param max_cnt	maximum number of states
 * @return		0 on success
 */
int dfa_alloc2(struct dfa *dfa, size_t max_cnt);

/**
 * Update maximum DFA size parameter.
 *
 * Explicitly update maximum DFA size parameter. Must be no less than
 * the current number of states
 *
 * @param dfa		pointer to the dfa structure
 * @param max_cnt	new maximum number of states
 * @return		0 on success
 */
int dfa_change_max_size(struct dfa *dfa, size_t max_cnt);

/**
 * Deinitialization of DFA structure.
 *
 * Just freeing memory allocated for the DFA inner fields.
 *
 * @param dfa	pointer to the dfa structure
 */
void dfa_free(struct dfa *dfa);

/**
 * Join two DFA.
 *
 * Joins two dfa - dst and src (dst|src) and saves the result to dst.
 *
 * @param dst	pointer to the dfa structure where result will be stored
 * @param src	pointer to the dfa structure that will be joined to the dst
 * @return	0 on success
 */
int dfa_join(struct dfa *dst, struct dfa *src);

/**
 * Append one DFA to another.
 *
 * Appends the second dfa to the first with 'anything between' (first .* second)
 * and saves the result to first.
 *
 * @param first		pointer to the dfa structure where result will be stored
 * @param second	pointer to the dfa structure that will be appended to the first
 * @return		0 on success
 */
int dfa_append(struct dfa *first, struct dfa *second);

/**
 * Minimize DFA.
 *
 * Minimizes number of states of the provided DFA.
 *
 * @param dfa	pointer to the dfa structure that will be minimized
 * @return	0 on success
 */
int dfa_minimize(struct dfa *dfa);

/**
 * Compress DFA representation.
 *
 * Compresses dfa inner data representation to minimize consumed memory.
 *
 * @param dfa	pointer to the dfa structure which memory will be minimized
 * @return	0 on success
 */
int dfa_compress(struct dfa *dfa);

/**
 * Add transition to DFA.
 *
 * Add transition between two states.
 *
 * @param dfa	pointer to the dfa structure where transition will be added
 * @param from	left state's index
 * @param mark	label of transition
 * @param to	right state's index
 * @return	0 on success
 */
int dfa_add_trans(struct dfa *dfa, size_t from, unsigned char mark, size_t to);

/**
 * Get DFA transitions's destination.
 *
 * Derives destination of the transition by source state's index and
 * transition's mark.
 *
 * @param dfa	pointer to the dfa structure where transition will be added
 * @param from	lef state's index
 * @param mark	label of transition
 * @return	index of the destination's state
 */
size_t dfa_get_trans(struct dfa *dfa, size_t from, unsigned char mark);

/**
 * Check if DFA's state is an accepting (final) state.
 *
 * Checks if the state with provided index is marked as final.
 *
 * @param dfa	pointer to the dfa structure
 * @param state	index of the DFA's state
 * @return	0 if the state with provided index is final
 */
int dfa_state_is_final(struct dfa *dfa, size_t state);

/**
 * Mark or unmark the DFA's state as a final state.
 *
 * Marks the state with provided index as final or remove this mark.
 *
 * @param dfa	pointer to the dfa structure
 * @param state	index of the DFA's state
 * @param final	0 to clear 'final' mark and 1 to set
 * @return	0 on success
 */
int dfa_state_set_final(struct dfa *dfa, size_t state, int final);

/**
 * Check if the DFA's state is a 'deadend'.
 *
 * Checks if all transitions from the state are go to that state.
 * So another state can never be reached.
 *
 * @param dfa	pointer to the dfa structure
 * @param state	index of the DFA's state
 * @return	0 if the state with provided index is a 'deadend'
 */
int dfa_state_is_deadend(struct dfa *dfa, size_t state);

/**
 * Add new state to the DFA.
 *
 * Adds new empty state to the DFA and returns the newly created state's index.
 *
 * @param dfa	pointer to the dfa structure
 * @param index	place where new index will be saved if not NULL
 * @return	0 on success
 */
int dfa_add_state(struct dfa *dfa, size_t *index);

/**
 * Add multiple new states to the DFA.
 *
 * Adds multiple new empty states to the DFA and returns index of the first
 * newly created state.
 *
 * @param dfa	pointer to the dfa structure
 * @param cnt	how many states must be added
 * @param index	place where new index will be saved if not NULL
 * @return	0 on success
 */
int dfa_add_n_state(struct dfa *dfa, size_t cnt, size_t *index);

/**
 * Save the dfa to the file.
 *
 * Saves the DFA structure to the given file.
 *
 * @param dfa		pointer to the dfa structure
 * @param filename	path where dfa structure must be saved
 * @return		0 on success
 */
int dfa_save_to_file(struct dfa *dfa, char *filename);

/**
 * Load the dfa from the file.
 *
 * Loads the DFA structure from the given file.
 *
 * @param dfa		pointer to the dfa structure
 * @param filename	path to the file with the saved dfa
 * @return		0 on success
 */
int dfa_load_from_file(struct dfa *dfa, char *filename);

#endif /** REFA_DFA_H @} */
