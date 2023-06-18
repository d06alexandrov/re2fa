/*
 * Conversion of regexp_tree to nfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

/**
 * @addtogroup conversion conversion
 * @{
 */

#ifndef REFA_TREE_TO_NFA_H
#define REFA_TREE_TO_NFA_H

#include "nfa.h"
#include "parser.h"

/**
 * Converting regexp tree to NFA.
 *
 * Converts regexp tree to initialized empty NFA with lambda-transitions.
 *
 * @param nfa		pointer to the existing and initialized empty NFA
 * @param re_tree	pointer to the source regexp tree
 * @return		0 on success
 */
int convert_tree_to_lambdanfa(struct nfa *nfa, struct regexp_tree *re_tree);

#endif /** REFA_TREE_TO_NFA_H @} */
