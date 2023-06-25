check_PROGRAMS =
TESTS =

if WITH_GTEST
check_PROGRAMS += re_tree_test

re_tree_test_SOURCES = re_tree.cpp
re_tree_test_CPPFLAGS = \
	-I$(top_srcdir)/lib
re_tree_test_LDADD = \
	$(top_builddir)/lib/librefa.la \
	$(GTEST_LIBS)

TESTS += re_tree_test
endif

if WITH_BENCHMARK
check_PROGRAMS += dfa_bench_test

dfa_bench_test_SOURCES = dfa_bench.cpp
dfa_bench_test_CPPFLAGS = \
	-I$(top_srcdir)/lib
dfa_bench_test_LDADD = \
	$(top_builddir)/lib/librefa.la \
	$(BENCHMARK_LIBS)

TESTS += dfa_bench_test
endif
