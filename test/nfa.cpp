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

TEST(nfaTests, add_state) {
	struct nfa nfa;
	int result;

	nfa_alloc(&nfa);

	result = nfa_add_node(&nfa, NULL);
	ASSERT_EQ(result, 0) <<
	"Failed to add state";
	result = nfa_add_node(&nfa, NULL);
	ASSERT_EQ(result, 0) <<
	"Failed to add state";
	EXPECT_EQ(nfa_state_count(&nfa), 2) <<
	"Added 2 states but result NFA has only " << nfa.node_cnt;

	nfa_free(&nfa);
}

TEST(nfaTests, add_multiple_states) {
	struct nfa nfa;
	int result;

	nfa_alloc(&nfa);

	result = nfa_add_node_n(&nfa, 2, NULL);
	ASSERT_EQ(result, 0) <<
	"Failed to add 2 states";
	EXPECT_EQ(nfa_state_count(&nfa), 2) <<
	"Added 2 states but result NFA has only " << nfa.node_cnt;

	nfa_free(&nfa);
}

TEST(nfaTests, add_transition) {
	struct nfa nfa;
	int result;
	size_t index;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);

	result = nfa_add_trans(&nfa, index, 'a', index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add transition between states";
	ASSERT_EQ(nfa_get_trans(&nfa, index, 'a', &trans_list), 1) <<
	"State " << index << " does not have transitions by 'a'";
	EXPECT_EQ(trans_list[0], index + 1) <<
	"State " << index <<
	" does not have transition by 'a' to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, add_same_transition) {
	struct nfa nfa;
	int result;
	size_t index;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_add_trans(&nfa, index, 'a', index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add transition between states";
	ASSERT_EQ(nfa_get_trans(&nfa, index, 'a', &trans_list), 1) <<
	"State " << index << " must have 1 transition by 'a'";
	EXPECT_EQ(trans_list[0], index + 1) <<
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
	EXPECT_EQ(nfa_get_trans(&nfa, index, 'a', NULL), 0) <<
	"State " << index << " still have transitions by 'a'";

	nfa_free(&nfa);
}


TEST(nfaTests, remove_nonexistent_transition) {
	struct nfa nfa;
	int result;
	size_t index;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_remove_trans(&nfa, index, 'b', index + 1);
	ASSERT_NE(result, 0) <<
	"Succed to remove nonexistent transition between states";
	ASSERT_EQ(nfa_get_trans(&nfa, index, 'b', NULL), 0) <<
	"State " << index << " must not have transitions by 'b'";
	ASSERT_EQ(nfa_get_trans(&nfa, index, 'a', &trans_list), 1) <<
	"State " << index << " must have 1 transition by 'a'";
	EXPECT_EQ(trans_list[0], index + 1) <<
	"State " << index <<
	" does not have transition by 'a' to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, remove_transition_nonexistent_state) {
	struct nfa nfa;
	int result;
	size_t index;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_remove_trans(&nfa, index + 2, 'a', index);
	ASSERT_NE(result, 0) <<
	"Succed to remove transition from nonexistent state";
	ASSERT_EQ(nfa_get_trans(&nfa, index, 'a', &trans_list), 1) <<
	"State " << index << " must have 1 transition by 'a'";
	EXPECT_EQ(trans_list[0], index + 1) <<
	"State " << index <<
	" does not have transition by 'a' to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, add_lambda_transition) {
	struct nfa nfa;
	int result;
	size_t index;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);

	result = nfa_add_lambda_trans(&nfa, index, index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add lambda-transition between states";
	ASSERT_EQ(nfa_get_lambda_trans(&nfa, index, &trans_list), 1) <<
	"State " << index << " does not have lambda-transitions";
	EXPECT_EQ(trans_list[0], index + 1) <<
	"State " << index <<
	" does not have lambda-transition to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, add_same_lambda_transition_fragile) {
	struct nfa nfa;
	int result;
	size_t index;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_lambda_trans(&nfa, index, index + 1);

	result = nfa_add_lambda_trans(&nfa, index, index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add lambda-transition between states";
	ASSERT_EQ(nfa_get_lambda_trans(&nfa, index, &trans_list), 1) <<
	"State " << index << " must have 1 lambda-transitions";
	EXPECT_EQ(trans_list[0], index + 1) <<
	"State " << index <<
	" does not have lambda-transition to " << index + 1;

	nfa_free(&nfa);
}

TEST(nfaTests, remove_all_transitions) {
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
	EXPECT_EQ(nfa_get_trans(&nfa, index, 'a', NULL), 0) <<
	"State " << index << " still has transitions by 'a'";
	EXPECT_EQ(nfa_get_lambda_trans(&nfa, index, NULL), 0) <<
	"State " << index << " still has lambda-transitions";
	EXPECT_EQ(nfa_get_trans(&nfa, index + 1, 'b', NULL), 1) <<
	"State " << index + 1 << " does not have transitions by 'b'";

	nfa_free(&nfa);
}

TEST(nfaTests, copy_transitions) {
	struct nfa nfa;
	int result;
	size_t index;
	size_t *trans_list;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);
	nfa_add_lambda_trans(&nfa, index, index + 1);
	nfa_add_trans(&nfa, index + 1, 'b', index);

	result = nfa_copy_trans(&nfa, index + 1, index);
	ASSERT_EQ(result, 0) <<
	"Failed to copy transitions from state " << index;
	ASSERT_EQ(nfa_get_trans(&nfa, index + 1, 'a', &trans_list), 1) <<
	"State " << index << " does not have 1 transition by 'a'";
	EXPECT_EQ(trans_list[0], index + 1) <<
	"State " << index + 1 <<
	" does not have transition by 'a' to " << index;
	EXPECT_EQ(nfa_get_lambda_trans(&nfa, index + 1, NULL), 0) <<
	"State " << index + 1 << " must not have lambda-transitions";
	ASSERT_EQ(nfa_get_trans(&nfa, index + 1, 'b', &trans_list), 1) <<
	"State " << index + 1 << " does not have 1 transition by 'b'";
	EXPECT_EQ(trans_list[0], index) <<
	"State " << index + 1 <<
	" does not have transition by 'b' to " << index;

	nfa_free(&nfa);
}

TEST(nfaTests, set_final) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_trans(&nfa, index, 'a', index + 1);

	result = nfa_state_set_final(&nfa, index + 1, 1);
	ASSERT_EQ(result, 0) <<
	"Failed to mark state " << index + 1 << " as final";
	EXPECT_FALSE(nfa_state_is_final(&nfa, index)) <<
	"State " << index << " must not be a final state";
	EXPECT_TRUE(nfa_state_is_final(&nfa, index + 1)) <<
	"State " << index + 1 << "must be a final state";

	nfa_free(&nfa);
}

TEST(nfaTests, reset_final) {
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
	EXPECT_FALSE(nfa_state_is_final(&nfa, index)) <<
	"State " << index << " must not be a final state";
	EXPECT_FALSE(nfa_state_is_final(&nfa, index + 1)) <<
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

TEST(nfaTests, rebuild1) {
	struct nfa nfa;
	int result;
	size_t index;

	nfa_alloc(&nfa);
	nfa_add_node_n(&nfa, 2, &index);
	nfa_add_lambda_trans(&nfa, index, index + 1);

	result = nfa_rebuild(&nfa);
	ASSERT_EQ(result, 0) <<
	"Failed to rebuild NFA";
	EXPECT_EQ(nfa_state_count(&nfa), 1) <<
	"After rebuild NFA must have only 1 state instead of " << nfa_state_count(&nfa);

	nfa_free(&nfa);
}
/*
TEST(nfaTests, rebuild2) {
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
	ASSERT_EQ(nfa_state_count(&nfa), 1) <<
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

	index = nfa_get_initial_state(&nfa1);
	EXPECT_EQ(nfa_get_trans(&nfa1, index, 'a', NULL), 1) <<
	"State " << index << " does not have transitions by 'a'";
	EXPECT_EQ(nfa_get_trans(&nfa1, index, 'b', NULL), 1) <<
	"State " << index << " does not have transitions by 'b'";

	nfa_free(&nfa1);
	nfa_free(&nfa2);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
