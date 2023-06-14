/*
 * Declaration of deterministic finite automaton.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef __DFA_H
#define __DFA_H

#include <stdint.h>
#include <stddef.h>

#define DFA_FLAG_LAST		(0x01)
#define DFA_FLAG_DEADEND	(0x02)

struct dfa {
	char	*comment; /* char string with any info */
	size_t	comment_size; /* size of allocated memory */

	size_t	state_cnt; /* number of dfa states */
	size_t	state_malloc_cnt; /* allocated dfa states (>= state_cnt) */

	uint8_t		bps; /* bits per state index */
	size_t		state_size; /* bytes per state */
	uint64_t	state_max_cnt; /* maximum size of dfa (0xFF..FF by default) */

	void	*trans; /* array of transitions (q -a-> trans[q][a]) */
	uint8_t	*flags; /* array of states' flags such as LAST and DEADEND */

	size_t	first_index;	/* initial state's index */

	void	*external; /* unknown */
	size_t	external_size; /* unknown */
};

/* dfa initialization */
int dfa_alloc(struct dfa *dst);
/* dfa initialization, @2 -- maximum size of dfa */
int dfa_alloc2(struct dfa *dfa, size_t max_cnt);
int dfa_change_max_size(struct dfa *dfa, size_t max_cnt);
/* dfa clear */
void dfa_free(struct dfa *dfa);
/* dfa copy */
//int dfa_copy(struct dfa *, struct dfa *);
/* joins dst and src (dst|src) and saves to dst */
int dfa_join(struct dfa *dst, struct dfa *src);
/* appends second dfa to first dfa
 * result: first = first .* second
 */
int dfa_append(struct dfa *first, struct dfa *second);
/* rebuild dfa to decrease state count */
int dfa_minimize(struct dfa *dst);
/* add node to dfa, returns new index to size_t ptr */
//int dfa_add_node(struct dfa *, size_t *);
/* compress dfa to minimize consumed memory */
int dfa_compress(struct dfa *);
/* add transition to dfa
 * @2 --@3--> @4
 */
int dfa_add_trans(struct dfa *, size_t, unsigned char, size_t);
/* get transition of dfa
 * returns @2 --@3-->
 */
size_t dfa_get_trans(struct dfa *, size_t, unsigned char);

int dfa_state_is_last(struct dfa *, size_t);
int dfa_state_set_last(struct dfa *, size_t, int);
/* state is a deadend if (state -a-> state) for every a */
int dfa_state_is_deadend(struct dfa *dfa, size_t state);
int dfa_state_set_deadend(struct dfa *, size_t, int); /* for inner purposes only */
int dfa_state_calc_deadend(struct dfa *, size_t);
/* add state to dfa and save index to @2*/
int dfa_add_state(struct dfa *, size_t *);
/* add @2 states to dfa and save first index to @3 */
int dfa_add_n_state(struct dfa *, size_t, size_t *);

/* saves dfa to file @2 */
int dfa_save_to_file(struct dfa *, char *);
/* loads dfa from file @2 */
int dfa_load_from_file(struct dfa *, char *);


/* for debug purposes */
void dfa_print(struct dfa *);
/* node initialization */
//int dfa_node_alloc(struct dfa_node *);
/* node clear */
//void dfa_node_clear(struct dfa_node *);

#endif
