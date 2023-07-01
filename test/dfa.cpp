#include <gtest/gtest.h>

extern "C" {
#include <refa.h>
}

TEST(dfaTests, allocation) {
	struct dfa dfa;
	int result;

	result = dfa_alloc(&dfa);
	ASSERT_EQ(result, 0) <<
	"Failed to allocate DFA";

	dfa_free(&dfa);
}

TEST(dfaTests, allocation2_fragile) {
	struct dfa dfa;
	int result;

	result = dfa_alloc2(&dfa, 255);
	ASSERT_EQ(result, 0) <<
	"Failed to allocate DFA";
	EXPECT_EQ(dfa.bps, 8) <<
	"DFA's bps is equal to " << dfa.bps << " instead of 8";

	dfa_free(&dfa);
}

TEST(dfaTests, allocation2_16bps_fragile) {
	struct dfa dfa;
	int result;

	result = dfa_alloc2(&dfa, 256);
	ASSERT_EQ(result, 0) <<
	"Failed to allocate DFA";
	EXPECT_EQ(dfa.bps, 16) <<
	"DFA's bps is equal to " << dfa.bps << " instead of 16";

	dfa_free(&dfa);
}

TEST(dfaTests, add_state_fragile) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc(&dfa);

	result = dfa_add_state(&dfa, &index);
	ASSERT_EQ(result, 0) <<
	"Failed to add state to DFA";
	result = dfa_add_state(&dfa, &index);
	ASSERT_EQ(result, 0) <<
	"Failed to add state to DFA";
	EXPECT_EQ(dfa.state_cnt, 2) <<
	"DFA must have 2 state instead of " << dfa.state_cnt;

	dfa_free(&dfa);
}

TEST(dfaTests, add_multiple_states_fragile) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc(&dfa);

	result = dfa_add_n_state(&dfa, 2, &index);
	ASSERT_EQ(result, 0) <<
	"Failed to add 2 states to DFA";
	EXPECT_EQ(dfa.state_cnt, 2) <<
	"DFA must have 2 state instead of " << dfa.state_cnt;

	dfa_free(&dfa);
}

TEST(dfaTests, add_transition_fragile) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc(&dfa);
	dfa_add_n_state(&dfa, 2, &index);

	result = dfa_add_trans(&dfa, index, 'a', index + 1);
	ASSERT_EQ(result, 0) <<
	"Failed to add transition from " << index << " to " << index + 1;
	EXPECT_EQ(dfa_get_trans(&dfa, index, 'a'), index + 1) <<
	"Transition by 'a' from " << index <<
	" goes to " << dfa_get_trans(&dfa, index, 'a') <<
	" instead of " << index + 1;

	dfa_free(&dfa);
}

TEST(dfaTests, keep_max_size_fragile) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc2(&dfa, 255);
	dfa_add_n_state(&dfa, 2, &index);
	dfa_add_trans(&dfa, index, 'a', index + 1);

	result = dfa_change_max_size(&dfa, 255);
	ASSERT_EQ(result, 0) <<
	"Failed to change DFA max size";
	EXPECT_EQ(dfa.bps, 8) <<
	"DFA's bps is equal to " << dfa.bps << " instead of 8";
	EXPECT_EQ(dfa_get_trans(&dfa, index, 'a'), index + 1) <<
	"Transition by 'a' from " << index <<
	" goes to " << dfa_get_trans(&dfa, index, 'a') <<
	" instead of " << index + 1;

	dfa_free(&dfa);
}

TEST(dfaTests, increase_max_size_fragile) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc2(&dfa, 255);
	dfa_add_n_state(&dfa, 2, &index);
	dfa_add_trans(&dfa, index, 'a', index + 1);

	result = dfa_change_max_size(&dfa, 256);
	ASSERT_EQ(result, 0) <<
	"Failed to change DFA max size";
	EXPECT_EQ(dfa.bps, 16) <<
	"DFA's bps is equal to " << dfa.bps << " instead of 16";
	EXPECT_EQ(dfa_get_trans(&dfa, index, 'a'), index + 1) <<
	"Transition by 'a' from " << index <<
	" goes to " << dfa_get_trans(&dfa, index, 'a') <<
	" instead of " << index + 1;

	dfa_free(&dfa);
}

TEST(dfaTests, decrease_max_size_fragile) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc2(&dfa, 256);
	dfa_add_n_state(&dfa, 2, &index);
	dfa_add_trans(&dfa, index, 'a', index + 1);

	result = dfa_change_max_size(&dfa, 255);
	ASSERT_EQ(result, 0) <<
	"Failed to change DFA max size";
	EXPECT_EQ(dfa.bps, 8) <<
	"DFA's bps is equal to " << dfa.bps << " instead of 8";
	EXPECT_EQ(dfa_get_trans(&dfa, index, 'a'), index + 1) <<
	"Transition by 'a' from " << index <<
	" goes to " << dfa_get_trans(&dfa, index, 'a') <<
	" instead of " << index + 1;

	dfa_free(&dfa);
}

TEST(dfaTests, compress_fragile) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc2(&dfa, 65536);
	dfa_add_n_state(&dfa, 2, &index);
	for (unsigned int i = 0; i < 256; i++) {
		dfa_add_trans(&dfa, index, i, index + 1);
		dfa_add_trans(&dfa, index + 1, i, index);
	}

	result = dfa_compress(&dfa);
	ASSERT_EQ(result, 0) <<
	"Failed to compress DFA";
	EXPECT_EQ(dfa.bps, 8) <<
	"DFA's bps is equal to " << dfa.bps << " instead of 8";
	dfa_free(&dfa);
}

TEST(dfaTests, set_final) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc(&dfa);
	dfa_add_n_state(&dfa, 2, &index);
	for (unsigned int i = 0; i < 256; i++) {
		dfa_add_trans(&dfa, index, i, index + 1);
		dfa_add_trans(&dfa, index + 1, i, index);
	}

	result = dfa_state_set_final(&dfa, index + 1, 1);
	ASSERT_EQ(result, 0) <<
	"Failed to compress DFA";
	EXPECT_FALSE(dfa_state_is_final(&dfa, index)) <<
	"State " << index << " must not be a final state";
	EXPECT_TRUE(dfa_state_is_final(&dfa, index + 1)) <<
	"State " << index + 1 << "must be a final state";

	dfa_free(&dfa);
}

TEST(dfaTests, reset_final) {
	struct dfa dfa;
	int result;
	size_t index;

	dfa_alloc(&dfa);
	dfa_add_n_state(&dfa, 2, &index);
	for (unsigned int i = 0; i < 256; i++) {
		dfa_add_trans(&dfa, index, i, index + 1);
		dfa_add_trans(&dfa, index + 1, i, index);
	}
	dfa_state_set_final(&dfa, index + 1, 1);

	result = dfa_state_set_final(&dfa, index + 1, 0);
	ASSERT_EQ(result, 0) <<
	"Failed to compress DFA";
	EXPECT_FALSE(dfa_state_is_final(&dfa, index)) <<
	"State " << index << " must not be a final state";
	EXPECT_FALSE(dfa_state_is_final(&dfa, index + 1)) <<
	"State " << index + 1 << "must not be a final state";

	dfa_free(&dfa);
}

TEST(dfaTests, join_fragile) {
	struct dfa dfa1, dfa2;
	int result;
	size_t index;

	dfa_alloc(&dfa1);
	dfa_add_n_state(&dfa1, 2, &index);
	for (unsigned int i = 0; i < 256; i++) {
		dfa_add_trans(&dfa1, index, i, index);
		dfa_add_trans(&dfa1, index + 1, i, index);
	}
	dfa_add_trans(&dfa1, index, 'a', index + 1);
	dfa_state_set_final(&dfa1, index + 1, 1);

	dfa_alloc(&dfa2);
	dfa_add_n_state(&dfa2, 2, &index);
	for (unsigned int i = 0; i < 256; i++) {
		dfa_add_trans(&dfa2, index, i, index);
		dfa_add_trans(&dfa2, index + 1, i, index);
	}
	dfa_add_trans(&dfa2, index, 'b', index + 1);
	dfa_state_set_final(&dfa2, index + 1, 1);

	result = dfa_join(&dfa1, &dfa2);
	ASSERT_EQ(result, 0) <<
	"Failed to join DFA";
	index = dfa_get_trans(&dfa1, dfa1.first_index, 'a');
	EXPECT_TRUE(dfa_state_is_final(&dfa1, index)) <<
	"Transition by 'a' must lead to a final state";
	index = dfa_get_trans(&dfa1, dfa1.first_index, 'b');
	EXPECT_TRUE(dfa_state_is_final(&dfa1, index)) <<
	"Transition by 'b' must lead to a final state";

	dfa_free(&dfa1);
	dfa_free(&dfa2);
}

TEST(dfaTests, append_fragile) {
	struct dfa dfa1, dfa2;
	int result;
	size_t index;

	dfa_alloc(&dfa1);
	dfa_add_n_state(&dfa1, 2, &index);
	for (unsigned int i = 0; i < 256; i++) {
		dfa_add_trans(&dfa1, index, i, index);
		dfa_add_trans(&dfa1, index + 1, i, index);
	}
	dfa_add_trans(&dfa1, index, 'a', index + 1);
	dfa_state_set_final(&dfa1, index + 1, 1);

	dfa_alloc(&dfa2);
	dfa_add_n_state(&dfa2, 2, &index);
	for (unsigned int i = 0; i < 256; i++) {
		dfa_add_trans(&dfa2, index, i, index);
		dfa_add_trans(&dfa2, index + 1, i, index);
	}
	dfa_add_trans(&dfa2, index, 'b', index + 1);
	dfa_state_set_final(&dfa2, index + 1, 1);

	result = dfa_append(&dfa1, &dfa2);
	ASSERT_EQ(result, 0) <<
	"Failed to append DFA";
	index = dfa_get_trans(&dfa1, dfa1.first_index, 'a');
	EXPECT_FALSE(dfa_state_is_final(&dfa1, index)) <<
	"Transition by 'a' must not lead to a final state";
	index = dfa_get_trans(&dfa1, index, 'b');
	EXPECT_TRUE(dfa_state_is_final(&dfa1, index)) <<
	"Transition by 'ab' must lead to a final state";

	dfa_free(&dfa1);
	dfa_free(&dfa2);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
