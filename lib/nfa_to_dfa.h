/*
 * Conversion of nfa to dfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

/**
 * @addtogroup conversion conversion
 * @{
 */

#ifndef REFA_NFA_TO_DFA_H
#define REFA_NFA_TO_DFA_H

#include "dfa.h"
#include "nfa.h"

/**
 * Converting lambda-free NFA to DFA.
 *
 * Converts NFA without lambda transitions to initialized empty DFA.
 *
 * @param dfa	pointer to the existing and initialized empty DFA
 * @param nfa	pointer to the source NFA without lambda-transitions
 * @return	0 on success
 */
int convert_nfa_to_dfa(struct dfa *dfa, const struct nfa *nfa);

#endif /** REFA_NFA_TO_DFA_H @} */
