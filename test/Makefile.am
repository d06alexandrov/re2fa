check_PROGRAMS = re_tree_test nfa_test

re_tree_test_SOURCES = re_tree.cpp
re_tree_test_CPPFLAGS = \
	-I$(top_srcdir)/lib
re_tree_test_LDADD = \
	$(top_builddir)/lib/librefa.la \
	$(GTEST_LIBS)

nfa_test_SOURCES = nfa.cpp
nfa_test_CPPFLAGS = \
	-I$(top_srcdir)/lib
nfa_test_LDADD = \
	$(top_builddir)/lib/librefa.la \
	$(GTEST_LIBS)

TESTS = re_tree_test nfa_test

if WITH_GCOVR
test-coverage: check-am
	mkdir -p $(builddir)/coverage
	gcovr -r $(top_srcdir) $(top_builddir)/lib \
		--print-summary \
		--html-details=$(builddir)/coverage/index.html

check: test-coverage
endif
