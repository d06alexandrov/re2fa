/*
 * Conversion of dfa to nfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include "dfa_to_nfa.h"

#include "dfa.h"
#include "nfa.h"

int convert_dfa_to_nfa(struct nfa *nfa, struct dfa *dfa)
{
	nfa_add_node_n(nfa, dfa->state_cnt, NULL);
	nfa->first_index = dfa->first_index;

	for (size_t i = 0; i < dfa->state_cnt; i++) {
		int isfinal = dfa_state_is_final(dfa, i);
		nfa_state_set_final(nfa, i, isfinal);


		for (int a = 0; a < 256; a++) {
			uint64_t to;
			to = dfa_get_trans(dfa, i, a);
			nfa_add_trans(nfa, i, a, to);
		}
	}

	return 0;
}
