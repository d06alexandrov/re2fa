/*
 * Conversion of dfa to nfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

/**
 * @addtogroup conversion conversion
 * @{
 */

#ifndef REFA_DFA_TO_NFA_H
#define REFA_DFA_TO_NFA_H

#include "dfa.h"
#include "nfa.h"

/**
 * Converting DFA to NFA.
 *
 * Converts DFA to initialized empty NFA.
 *
 * @param nfa	pointer to the existing and initialized empty NFA
 * @param dfa	pointer to the source DFA
 * @return	0 on success
 */
int convert_dfa_to_nfa(struct nfa *nfa, struct dfa *dfa);

#endif /** REFA_DFA_TO_NFA_H @} */
