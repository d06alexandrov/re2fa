/*
 * Conversion of regexp_tree to nfa.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef __TREE_TO_NFA_H
#define __TREE_TO_NFA_H

#include "nfa.h"
#include "parser.h"

/*
 * @1 - pointer to EXISTING INITIALIZED nfa
 * @2 - pointer to regexp_tree
 * value: 0 if ok
 */
int convert_tree_to_lambdanfa(struct nfa *, struct regexp_tree *);

#endif
