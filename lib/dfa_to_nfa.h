/*
 * Conversion of dfa to nfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef __DFA_TO_NFA_H
#define __DFA_TO_NFA_H

#include "dfa.h"
#include "nfa.h"

/*
 * @1 - pointer to EXISTING INITIALIZED nfa
 * @2 - pointer to dfa
 * return: 0 if ok
 */
int convert_dfa_to_nfa(struct nfa *nfa, struct dfa *dfa);

#endif
