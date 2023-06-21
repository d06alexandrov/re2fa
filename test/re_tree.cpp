#include <gtest/gtest.h>

extern "C" {
#include <refa.h>
}

#include <iostream>

#define PARSE_CORRECT_REGEXP(re)			\
	do {						\
		struct regexp_tree *re_tree;		\
		re_tree = regexp_to_tree(re, NULL);	\
		ASSERT_NE(re_tree, nullptr) <<		\
		"Failed to parse correct regexp: " re;	\
		regexp_tree_free(re_tree);		\
	} while (0)

#define PARSE_INCORRECT_REGEXP(re)			\
	do {						\
		struct regexp_tree *re_tree;		\
		re_tree = regexp_to_tree(re, NULL);	\
		ASSERT_EQ(re_tree, nullptr) <<		\
		"Failed to parse incorrect regexp " re;	\
	} while (0)

/* Correct regular expressions */

TEST(re_treeTests, test_correct_simple) {
	PARSE_CORRECT_REGEXP("/abc/");
}

TEST(re_treeTests, test_correct_simple_mod) {
	PARSE_CORRECT_REGEXP("/abc/smi");
}

TEST(re_treeTests, test_correct_char_types_d) {
	PARSE_CORRECT_REGEXP("/a\\db\\Dc/");
}

TEST(re_treeTests, test_correct_char_types_h) {
	PARSE_CORRECT_REGEXP("/a\\hb\\Hc/");
}

TEST(re_treeTests, test_correct_char_types_s) {
	PARSE_CORRECT_REGEXP("/a\\sb\\Sc/");
}

TEST(re_treeTests, test_correct_char_types_v) {
	PARSE_CORRECT_REGEXP("/a\\vb\\Vc/");
}

TEST(re_treeTests, test_correct_char_types_w) {
	PARSE_CORRECT_REGEXP("/a\\wb\\Wc/");
}

#ifdef USE_POSIX_CHAR_CLASSES
TEST(re_treeTests, test_correct_posix_alnum) {
	PARSE_CORRECT_REGEXP("/a[:alnum:]c/");
}

TEST(re_treeTests, test_correct_posix_alpha) {
	PARSE_CORRECT_REGEXP("/a[:alpha:]c/");
}

TEST(re_treeTests, test_correct_posix_ascii) {
	PARSE_CORRECT_REGEXP("/a[:ascii:]c/");
}

TEST(re_treeTests, test_correct_posix_blank) {
	PARSE_CORRECT_REGEXP("/a[:blank:]c/");
}

TEST(re_treeTests, test_correct_posix_cntrl) {
	PARSE_CORRECT_REGEXP("/a[:cntrl:]c/");
}

TEST(re_treeTests, test_correct_posix_digit) {
	PARSE_CORRECT_REGEXP("/a[:digit:]c/");
}

TEST(re_treeTests, test_correct_posix_graph) {
	PARSE_CORRECT_REGEXP("/a[:graph:]c/");
}

TEST(re_treeTests, test_correct_posix_lower) {
	PARSE_CORRECT_REGEXP("/a[:lower:]c/");
}

TEST(re_treeTests, test_correct_posix_print) {
	PARSE_CORRECT_REGEXP("/a[:print:]c/");
}

TEST(re_treeTests, test_correct_posix_punct) {
	PARSE_CORRECT_REGEXP("/a[:punct:]c/");
}

TEST(re_treeTests, test_correct_posix_space) {
	PARSE_CORRECT_REGEXP("/a[:space:]c/");
}

TEST(re_treeTests, test_correct_posix_upper) {
	PARSE_CORRECT_REGEXP("/a[:upper:]c/");
}

TEST(re_treeTests, test_correct_posix_word) {
	PARSE_CORRECT_REGEXP("/a[:word:]c/");
}

TEST(re_treeTests, test_correct_posix_xdigit) {
	PARSE_CORRECT_REGEXP("/a[:xdigit:]c/");
}
#endif /* USE_POSIX_CHAR_CLASSES */

TEST(re_treeTests, test_correct_subpattern) {
	PARSE_CORRECT_REGEXP("/a(b)c/");
}

TEST(re_treeTests, test_correct_subpattern_alt) {
	PARSE_CORRECT_REGEXP("/a(b|d)c/");
}

TEST(re_treeTests, test_correct_subpattern_alt2) {
	PARSE_CORRECT_REGEXP("/a(b|d|)c/");
}

TEST(re_treeTests, test_correct_subpattern_alt3) {
	PARSE_CORRECT_REGEXP("/a(b|d|)c/");
}

TEST(re_treeTests, test_correct_minmax1) {
	PARSE_CORRECT_REGEXP("/ab{5}c/");
}

TEST(re_treeTests, test_correct_minmax2) {
	PARSE_CORRECT_REGEXP("/ab{5,65535}c/");
}

TEST(re_treeTests, test_correct_minmax3) {
	PARSE_CORRECT_REGEXP("/ab{5,}c/");
}

TEST(re_treeTests, test_correct_minmax_chars) {
	PARSE_CORRECT_REGEXP("/ab}c/");
}

TEST(re_treeTests, test_correct_minmax_chars2) {
	PARSE_CORRECT_REGEXP("/ab{,6}c/");
}

TEST(re_treeTests, test_correct_minmax_plus) {
	PARSE_CORRECT_REGEXP("/ab+c/");
}

TEST(re_treeTests, test_correct_minmax_star) {
	PARSE_CORRECT_REGEXP("/ab*c/");
}

TEST(re_treeTests, test_correct_minmax_question) {
	PARSE_CORRECT_REGEXP("/ab?c/");
}

/* Incorrect regular expressions */

TEST(re_treeTests, test_incorrect_end) {
	PARSE_INCORRECT_REGEXP("/abc");
}

TEST(re_treeTests, test_incorrect_begin) {
	PARSE_INCORRECT_REGEXP("abc/");
}

TEST(re_treeTests, test_incorrect_ext) {
	PARSE_INCORRECT_REGEXP("/abc/smFi");
}

TEST(re_treeTests, test_incorrect_subpattern_left) {
	PARSE_INCORRECT_REGEXP("/a(bc/");
}

TEST(re_treeTests, test_incorrect_subpattern_right) {
	PARSE_INCORRECT_REGEXP("/ab)c/");
}

TEST(re_treeTests, test_incorrect_minmax1) {
	PARSE_INCORRECT_REGEXP("/ab{7,5}c/");
}

TEST(re_treeTests, test_incorrect_minmax2) {
	PARSE_INCORRECT_REGEXP("/ab{65536}c/");
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
