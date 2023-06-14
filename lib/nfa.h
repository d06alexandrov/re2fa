/*
 * Declaration of nondeterministic finite automaton.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef __NFA_H
#define __NFA_H

#include <stddef.h>

struct nfa {
	char		*comment; /* char string with any info */
	size_t		comment_size; /* size of allocated memory */

	size_t		node_cnt; /* number of nfa states */
	size_t		node_mem_size;	/* number of allocated nfa states */
	struct nfa_node	*nodes; /* array of nfa states */

	size_t		first_index;	/* first state */
};

/* Σ-alphabet power */

struct nfa_node {
	size_t	index;		/* node's index */
	size_t	islast;		/* if islast > 0, then last */

	size_t	self_closed;	/* closure */
	size_t	prelast;	/* any symbol trans to last */

	size_t	*lambda_trans;	/* lambda transitions */
	size_t	lambda_cnt;
	size_t	*trans[256];	/* regular transitions */
	size_t	trans_cnt[256];
};

/* nfa initialization */
int nfa_alloc(struct nfa *);
/* nfa clear */
void nfa_free(struct nfa *);
/* rebuild nfa to remove unreachable states and λ-transitions */
int nfa_rebuild(struct nfa *);
/* join nfa @2 to nfa @1 */
int nfa_join(struct nfa *, struct nfa *);
/* add node to nfa, returns new index to size_t ptr */
int nfa_add_node(struct nfa *, size_t *);
/* add n nodes to nfa, returns first new index to size_t ptr */
int nfa_add_node_n(struct nfa *, size_t *, size_t);

int nfa_state_is_last(struct nfa *, size_t);
int nfa_state_set_last(struct nfa *, size_t, int);
/* add λ-transition to nfa
 * @2 --> @3
 */
int nfa_add_lambda_trans(struct nfa *, size_t, size_t);
/* add transition to nfa
 * @2 --@3--> @4
 */
int nfa_add_trans(struct nfa *, size_t, unsigned char, size_t);
/* remove transition from nfa
 * @2 --@3--> @4
 */
int nfa_remove_trans(struct nfa *, size_t, unsigned char, size_t);
int nfa_remove_trans_all(struct nfa *, size_t);
/* copy all transitions from @3 to @2 */
int nfa_copy_trans(struct nfa *, size_t, size_t);

int nfa_remove_lambda(struct nfa *);

/*int nfa_lambda_reachable(size_t *dst, size_t *cnt, size_t from, struct nfa *src);*/


/* for debug purposes */
void nfa_print(struct nfa *);

/* @2th node initialization */
int nfa_node_alloc(struct nfa_node *, size_t);
/* node clear */
void nfa_node_free(struct nfa_node *);
/* add λ-transition */
int nfa_node_add_lambda_trans(struct nfa_node *, size_t);
/* add regular transition */
int nfa_node_add_trans(struct nfa_node *, unsigned char, size_t);
/* remove regular transition */
int nfa_node_remove_trans(struct nfa_node *, unsigned char, size_t);
/* remove all transitions from node */
int nfa_node_remove_trans_all(struct nfa_node *);

#endif
