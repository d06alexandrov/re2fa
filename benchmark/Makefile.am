check_PROGRAMS = dfa_bench_test

dfa_bench_test_SOURCES = dfa_bench.cpp
dfa_bench_test_CPPFLAGS = \
	-I$(top_srcdir)/lib
dfa_bench_test_LDADD = \
	$(top_builddir)/lib/librefa.la \
	$(BENCHMARK_LIBS)

TESTS = dfa_bench_test
