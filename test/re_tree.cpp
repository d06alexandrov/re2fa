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

TEST(re_treeTests, test_correct_simple_begin) {
	PARSE_CORRECT_REGEXP("/^abc/");
}

TEST(re_treeTests, test_correct_simple_end) {
	PARSE_CORRECT_REGEXP("/abc$/");
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

TEST(re_treeTests, test_correct_charclass_with_d) {
	PARSE_CORRECT_REGEXP("/a[\\d]b[\\D]c/");
}

TEST(re_treeTests, test_correct_charclass_with_h) {
	PARSE_CORRECT_REGEXP("/a[\\h]b[\\H]c/");
}

TEST(re_treeTests, test_correct_charclass_with_s) {
	PARSE_CORRECT_REGEXP("/a[\\s]b[\\S]c/");
}

TEST(re_treeTests, test_correct_charclass_with_v) {
	PARSE_CORRECT_REGEXP("/a[\\v]b[\\V]c/");
}

TEST(re_treeTests, test_correct_charclass_with_w) {
	PARSE_CORRECT_REGEXP("/a[\\w]b[\\W]c/");
}

TEST(re_treeTests, test_correct_charclass_with_w_mod) {
	PARSE_CORRECT_REGEXP("/a[\\w]c/i");
}

TEST(re_treeTests, test_correct_char_types_w_mod) {
	PARSE_CORRECT_REGEXP("/a\\wc/i");
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

TEST(re_treeTests, test_correct_any) {
	PARSE_CORRECT_REGEXP("/a.c/");
}

TEST(re_treeTests, test_correct_hex) {
	PARSE_CORRECT_REGEXP("/a\\x01\\xab\\xCd\\xeFc/");
}

TEST(re_treeTests, test_correct_charclass_with_hex) {
	PARSE_CORRECT_REGEXP("/a[\\x01\\xab\\xCd\\xeF]c/");
}

TEST(re_treeTests, test_correct_octet) {
	PARSE_CORRECT_REGEXP("/a\\012\\034\\156\\113c/");
}

TEST(re_treeTests, test_correct_charclass_with_octet) {
	PARSE_CORRECT_REGEXP("/a[\\012\\034\\156\\113]c/");
}

TEST(re_treeTests, test_correct_backslash_regular1) {
	PARSE_CORRECT_REGEXP("/a\\\\c/");
}

TEST(re_treeTests, test_correct_backslash_regular2) {
	PARSE_CORRECT_REGEXP("/a\\*c/");
}

TEST(re_treeTests, test_correct_charclass_backslash_regular) {
	PARSE_CORRECT_REGEXP("/a[\\\\]c/");
}

TEST(re_treeTests, test_correct_charclass_range1) {
	PARSE_CORRECT_REGEXP("/a[a-z]c/");
}

TEST(re_treeTests, test_correct_charclass_range2) {
	PARSE_CORRECT_REGEXP("/a[-z]c/");
}

TEST(re_treeTests, test_correct_charclass_range3) {
	PARSE_CORRECT_REGEXP("/a[a-]c/");
}

/* Incorrect regular expressions */

TEST(re_treeTests, test_incorrect_end) {
	PARSE_INCORRECT_REGEXP("/abc");
}

TEST(re_treeTests, test_incorrect_begin) {
	PARSE_INCORRECT_REGEXP("abc/");
}

TEST(re_treeTests, test_incorrect_multiple_ends) {
	PARSE_INCORRECT_REGEXP("/a/bc/");
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

TEST(re_treeTests, test_incorrect_minmax3) {
	PARSE_INCORRECT_REGEXP("/ab{123456}c/");
}

TEST(re_treeTests, test_incorrect_minmax4) {
	PARSE_INCORRECT_REGEXP("/ab{2,123456}c/");
}

TEST(re_treeTests, test_incorrect_hex1) {
	PARSE_INCORRECT_REGEXP("/a\\xT1c/");
}

TEST(re_treeTests, test_incorrect_hex2) {
	PARSE_INCORRECT_REGEXP("/a\\x.1c/");
}

TEST(re_treeTests, test_incorrect_charclass_with_hex1) {
	PARSE_INCORRECT_REGEXP("/a[\\xT1]c/");
}

TEST(re_treeTests, test_incorrect_charclass_with_hex2) {
	PARSE_INCORRECT_REGEXP("/a[\\x0]c/");
}

TEST(re_treeTests, test_incorrect_octet1) {
	PARSE_INCORRECT_REGEXP("/a\\455c/");
}

TEST(re_treeTests, test_incorrect_octet2) {
	PARSE_INCORRECT_REGEXP("/a\\0a1c/");
}

TEST(re_treeTests, test_incorrect_charclass_with_octet1) {
	PARSE_INCORRECT_REGEXP("/a[\\455]c/");
}

TEST(re_treeTests, test_incorrect_charclass_with_octet2) {
	PARSE_INCORRECT_REGEXP("/a[\\12]c/");
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
