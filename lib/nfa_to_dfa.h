/*
 * Conversion of nfa to dfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef __NFA_TO_DFA_H
#define __NFA_TO_DFA_H

#include "dfa.h"
#include "nfa.h"

/*
 * @1 - pointer to EXISTING INITIALIZED dfa
 * @2 - pointer to nonlambda nfa
 * value: 0 if ok
 */
int convert_nfa_to_dfa(struct dfa *, struct nfa *);

#endif
