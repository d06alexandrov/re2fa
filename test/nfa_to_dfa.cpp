#include <gtest/gtest.h>

extern "C" {
#include <refa.h>
}

TEST(nfa_to_dfaTests, simple_abc_fragile) {
	struct nfa nfa;
	struct dfa dfa;
	int result;
	unsigned int i;
	size_t index;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 4, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);
	nfa_add_trans(&nfa, index + 1, 'b', index + 2);
	nfa_add_trans(&nfa, index + 2, 'c', index + 3);
	for (i = 0; i < 256; i++) {
		nfa_add_trans(&nfa, index, i, index);
		nfa_add_trans(&nfa, index + 3, i, index + 3);
	}
	nfa_state_set_final(&nfa, index + 3, 1);
	dfa_alloc(&dfa);

	result = convert_nfa_to_dfa(&dfa, &nfa);

	ASSERT_EQ(result, 0) <<
	"Failed to build dfa by nfa";
	dfa_minimize(&dfa);
	EXPECT_EQ(dfa.state_cnt, 4) <<
	"Minimized DFA for '/abc/' must have 4 state instead of " << dfa.state_cnt;
	index = dfa_get_trans(&dfa, dfa.first_index, 'a');
	EXPECT_FALSE(dfa_state_is_final(&dfa, index)) <<
        "Transition by 'a' must lead to a non-final state";
	index = dfa_get_trans(&dfa, index, 'b');
	EXPECT_FALSE(dfa_state_is_final(&dfa, index)) <<
        "Transition by 'ab' must lead to a non-final state";
	index = dfa_get_trans(&dfa, index, 'c');
	EXPECT_TRUE(dfa_state_is_final(&dfa, index)) <<
        "Transition by 'abc' must lead to a final state";

	dfa_free(&dfa);
	nfa_free(&nfa);
}

TEST(nfa_to_dfaTests, a_pt_10c_fragile) {
	struct nfa nfa;
	struct dfa dfa;
	int result;
	unsigned int i;
	size_t index, j;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 13, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);
	for (j = index + 1; j < index + 11; j++) {
		for (i = 0; i < 256; i++) {
			nfa_add_trans(&nfa, j, i, j + 1);
		}
	}
	nfa_add_trans(&nfa, index + 11, 'c', index + 12);
	for (i = 0; i < 256; i++) {
		nfa_add_trans(&nfa, index, i, index);
		nfa_add_trans(&nfa, index + 12, i, index + 12);
	}
	nfa_state_set_final(&nfa, index + 12, 1);
	dfa_alloc(&dfa);

	result = convert_nfa_to_dfa(&dfa, &nfa);

	ASSERT_EQ(result, 0) <<
	"Failed to build dfa by nfa";
	dfa_minimize(&dfa);
	EXPECT_EQ(dfa.state_cnt, 2049) <<
	"Minimized DFA for '/a.{10}c/' must have 2049 state instead of " << dfa.state_cnt;
	index = dfa_get_trans(&dfa, dfa.first_index, 'a');
	for (i = 0; i < 10; i++) {
		index = dfa_get_trans(&dfa, index, 'b');
	}
	index = dfa_get_trans(&dfa, index, 'c');
	EXPECT_TRUE(dfa_state_is_final(&dfa, index)) <<
        "Transition by 'ab{10}c' must lead to a final state";

	dfa_free(&dfa);
	nfa_free(&nfa);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
