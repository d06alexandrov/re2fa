#include <gtest/gtest.h>

extern "C" {
#include <refa.h>
}

TEST(nfaTests, allocation) {
	struct nfa nfa;
	int result;

	result = nfa_alloc(&nfa);

	ASSERT_EQ(result, 0) <<
	"Failed to allocate NFA";

	nfa_free(&nfa);
}

TEST(nfaTests, free_null) {
	nfa_free(NULL);
}

TEST(nfaTests, add_state_fragile) {
	struct nfa nfa;
	int result;

	nfa_alloc(&nfa);

	result = nfa_add_node(&nfa, NULL);
	ASSERT_EQ(result, 0) <<
	"Failed to add state";
	result = nfa_add_node(&nfa, NULL);
	ASSERT_EQ(result, 0) <<
	"Failed to add state";
	EXPECT_EQ(nfa.node_cnt, 2) <<
	"Added 2 states but result NFA has only " << nfa.node_cnt;

	nfa_free(&nfa);
}

TEST(nfaTests, add_multiple_states_fragile) {
	struct nfa nfa;
	int result;

	nfa_alloc(&nfa);

	result = nfa_add_node_n(&nfa, 2, NULL);
	ASSERT_EQ(result, 0) <<
	"Failed to add 2 states";
	EXPECT_EQ(nfa.node_cnt, 2) <<
	"Added 2 states but result NFA has only " << nfa.node_cnt;

	nfa_free(&nfa);
}

TEST(nfaTests, add_transition_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);

	result = nfa_add_trans(&nfa, index, 'a', index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add transition between states";
	ASSERT_EQ(nfa.nodes[index].trans_cnt['a'], 1) <<
	"State " << index << " does not have transitions by 'a'";
	EXPECT_EQ(nfa.nodes[index].trans['a'][0], index + 1) <<
	"State " << index <<
	" does not have transition by 'a' to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, add_same_transition_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_add_trans(&nfa, index, 'a', index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add transition between states";
	ASSERT_EQ(nfa.nodes[index].trans_cnt['a'], 1) <<
	"State " << index <<
	" must not have 1 transition by 'a' instead of " << nfa.nodes[index].trans_cnt['a'];
	EXPECT_EQ(nfa.nodes[index].trans['a'][0], index + 1) <<
	"State " << index <<
	" does not have transition by 'a' to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, remove_transition_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_remove_trans(&nfa, index, 'a', index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to remove transition between states";
	EXPECT_EQ(nfa.nodes[index].trans_cnt['a'], 0) <<
	"State " << index << " still have transitions by 'a'";

	nfa_free(&nfa);
}


TEST(nfaTests, remove_nonexistent_transition_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_remove_trans(&nfa, index, 'b', index + 1);
	ASSERT_NE(result, 0) <<
	"Succed to remove nonexistent transition between states";
	ASSERT_EQ(nfa.nodes[index].trans_cnt['a'], 1) <<
	"State " << index << " does not have transitions by 'a'";
	EXPECT_EQ(nfa.nodes[index].trans['a'][0], index + 1) <<
	"State " << index <<
	" does not have transition by 'a' to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, remove_transition_nonexistent_state_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_remove_trans(&nfa, index + 2, 'a', index);
	ASSERT_NE(result, 0) <<
	"Succed to remove transition from nonexistent state";
	ASSERT_EQ(nfa.nodes[index].trans_cnt['a'], 1) <<
	"State " << index << " does not have transitions by 'a'";
	EXPECT_EQ(nfa.nodes[index].trans['a'][0], index + 1) <<
	"State " << index <<
	" does not have transition by 'a' to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, add_lambda_transition_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);

	result = nfa_add_lambda_trans(&nfa, index, index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add lambda-transition between states";
	ASSERT_EQ(nfa.nodes[index].lambda_cnt, 1) <<
	"State " << index << " does not have lambda-transitions";
	EXPECT_EQ(nfa.nodes[index].lambda_trans[0], index + 1) <<
	"State " << index <<
	" does not have lambda-transition to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, add_same_lambda_transition_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_lambda_trans(&nfa, index, index + 1);

	result = nfa_add_lambda_trans(&nfa, index, index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add lambda-transition between states";
	ASSERT_EQ(nfa.nodes[index].lambda_cnt, 1) <<
	"State " << index <<
	" must have 1 lambda-transition instead of " << nfa.nodes[index].lambda_cnt;
	EXPECT_EQ(nfa.nodes[index].lambda_trans[0], index + 1) <<
	"State " << index <<
	" does not have lambda-transition to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, remove_all_transitions_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);
	nfa_add_lambda_trans(&nfa, index, index + 1);
	nfa_add_trans(&nfa, index + 1, 'b', index);

	result = nfa_remove_trans_all(&nfa, index);
	ASSERT_EQ(result, 0) <<
	"Failed to remove all transitions from state " << index;
	EXPECT_EQ(nfa.nodes[index].trans_cnt['a'], 0) <<
	"State " << index << " still has transitions by 'a'";
	EXPECT_EQ(nfa.nodes[index].lambda_cnt, 0) <<
	"State " << index << " still has lambda-transitions";
	EXPECT_EQ(nfa.nodes[index + 1].trans_cnt['b'], 1) <<
	"State " << index + 1 << " does not have transitions by 'b'";

	nfa_free(&nfa);
}

TEST(nfaTests, copy_transitions_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);
	nfa_add_lambda_trans(&nfa, index, index + 1);
	nfa_add_trans(&nfa, index + 1, 'b', index);

	result = nfa_copy_trans(&nfa, index + 1, index);
	ASSERT_EQ(result, 0) <<
	"Failed to copy transitions from state " << index;
	ASSERT_EQ(nfa.nodes[index + 1].trans_cnt['a'], 1) <<
	"State " << index << " does not have 1 transition by 'a'";
	EXPECT_EQ(nfa.nodes[index + 1].trans['a'][0], index + 1) <<
	"State " << index + 1 <<
	" does not have transition by 'a' to " << index;
	ASSERT_EQ(nfa.nodes[index + 1].lambda_cnt, 0) <<
	"State " << index + 1 << " must not have 1 lambda-transition";
	ASSERT_EQ(nfa.nodes[index + 1].trans_cnt['b'], 1) <<
	"State " << index + 1 << " does not have 1 transition by 'b'";
	EXPECT_EQ(nfa.nodes[index + 1].trans['b'][0], index) <<
	"State " << index + 1 <<
	" does not have transition by 'b' to " << index;

	nfa_free(&nfa);
}

TEST(nfaTests, set_final_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_state_set_final(&nfa, index + 1, 1);
	ASSERT_EQ(result, 0) <<
	"Failed to mark state " << index + 1 << " as final";
	EXPECT_FALSE(nfa.nodes[index].isfinal) <<
	"State " << index << " must not be a final state";
	EXPECT_TRUE(nfa.nodes[index + 1].isfinal) <<
	"State " << index + 1 << "must be a final state";

	nfa_free(&nfa);
}

TEST(nfaTests, reset_final_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);
	nfa_state_set_final(&nfa, index + 1, 1);

	result = nfa_state_set_final(&nfa, index + 1, 0);
	ASSERT_EQ(result, 0) <<
	"Failed to mark state " << index + 1 << " as final";
	EXPECT_FALSE(nfa.nodes[index].isfinal) <<
	"State " << index << " must not be a final state";
	EXPECT_FALSE(nfa.nodes[index + 1].isfinal) <<
	"State " << index + 1 << "must be a final state";

	nfa_free(&nfa);
}

TEST(nfaTests, check_final) {
	struct nfa nfa;
	size_t index;
	bool final1, final2;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);
	nfa_state_set_final(&nfa, index + 1, 1);

	final1 = nfa_state_is_final(&nfa, index);
	final2 = nfa_state_is_final(&nfa, index + 1);

	EXPECT_FALSE(final1) <<
	"Check for final state returned " << final1 << " instead of false";
	EXPECT_TRUE(final2) <<
	"Check for final state returned " << final2 << " instead of true";

	nfa_free(&nfa);
}

TEST(nfaTests, rebuild1_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_lambda_trans(&nfa, index, index + 1);

	result = nfa_rebuild(&nfa);
	ASSERT_EQ(result, 0) <<
	"Failed to rebuild NFA";
	EXPECT_EQ(nfa.node_cnt, 1) <<
	"After rebuild NFA must have only 1 state instead of " << nfa.node_cnt;

	nfa_free(&nfa);
}
/*
TEST(nfaTests, rebuild2_fragile) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_lambda_trans(&nfa, index, index + 1);
	nfa_state_set_final(&nfa, index + 1, 1);

	result = nfa_rebuild(&nfa);

	ASSERT_EQ(result, 0) <<
	"Failed to rebuild NFA";
	ASSERT_EQ(nfa.node_cnt, 1) <<
	"After rebuild NFA must have only 1 state instead of " << nfa.node_cnt;
	ASSERT_TRUE(nfa_state_is_final(&nfa, index)) <<
	"After rebuild state " << index << " must become a final state";

	nfa_free(&nfa);
}*/

TEST(nfaTests, join_fragile) {
	struct nfa nfa1, nfa2;
	int result;
	size_t index;

	nfa_alloc(&nfa1);
	nfa_add_node_n(&nfa1, 2, &index);
	nfa_add_trans(&nfa1, index, 'a', index + 1);

	nfa_alloc(&nfa2);
	nfa_add_node_n(&nfa2, 2, &index);
	nfa_add_trans(&nfa2, index, 'b', index + 1);
	nfa_state_set_final(&nfa2, index + 1, 1);

	result = nfa_join(&nfa1, &nfa2);
	ASSERT_EQ(result, 0) <<
	"Failed to join NFA";

	nfa_remove_lambda(&nfa1);

	EXPECT_EQ(nfa1.nodes[nfa1.first_index].trans_cnt['a'], 1) <<
	"State " << index << " does not have transitions by 'a'";
	EXPECT_EQ(nfa1.nodes[nfa1.first_index].trans_cnt['b'], 1) <<
	"State " << index << " does not have transitions by 'b'";

	nfa_free(&nfa1);
	nfa_free(&nfa2);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
