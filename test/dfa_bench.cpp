#include <benchmark/benchmark.h>

extern "C" {
#include <refa.h>
}

static void build_dfa_blow1(benchmark::State& state) {
	struct regexp_tree *re_tree;
	struct nfa nfa;
	struct dfa dfa;

	re_tree = regexp_to_tree("/a.{6}b/", NULL);

	nfa_alloc(&nfa);
	convert_tree_to_lambdanfa(&nfa, re_tree);
	regexp_tree_free(re_tree);

	nfa_rebuild(&nfa);

	for (auto _ : state) {
		dfa_alloc(&dfa);
		convert_nfa_to_dfa(&dfa, &nfa);
		dfa_free(&dfa);
	}

	nfa_free(&nfa);
}

static void build_dfa_blow1_minimize(benchmark::State& state) {
	struct regexp_tree *re_tree;
	struct nfa nfa;
	struct dfa dfa;

	re_tree = regexp_to_tree("/a.{6}b/", NULL);

	nfa_alloc(&nfa);
	convert_tree_to_lambdanfa(&nfa, re_tree);
	regexp_tree_free(re_tree);

	nfa_rebuild(&nfa);

	for (auto _ : state) {
		dfa_alloc(&dfa);
		convert_nfa_to_dfa(&dfa, &nfa);
		dfa_minimize(&dfa);
		dfa_free(&dfa);
	}

	nfa_free(&nfa);
}

static void build_dfa_blow2(benchmark::State& state) {
	struct regexp_tree *re_tree;
	struct nfa nfa;
	struct dfa dfa;

	re_tree = regexp_to_tree("/(a.*b|c.*d|e.*f|g.*h|j.*k|l.*m)/", NULL);

	nfa_alloc(&nfa);
	convert_tree_to_lambdanfa(&nfa, re_tree);
	regexp_tree_free(re_tree);

	nfa_rebuild(&nfa);

	for (auto _ : state) {
		dfa_alloc(&dfa);
		convert_nfa_to_dfa(&dfa, &nfa);
		dfa_free(&dfa);
	}

	nfa_free(&nfa);
}

static void build_dfa_blow2_minimize(benchmark::State& state) {
	struct regexp_tree *re_tree;
	struct nfa nfa;
	struct dfa dfa;

	re_tree = regexp_to_tree("/(a.*b|c.*d|e.*f|g.*h|j.*k|l.*m)/", NULL);

	nfa_alloc(&nfa);
	convert_tree_to_lambdanfa(&nfa, re_tree);
	regexp_tree_free(re_tree);

	nfa_rebuild(&nfa);

	for (auto _ : state) {
		dfa_alloc(&dfa);
		convert_nfa_to_dfa(&dfa, &nfa);
		dfa_minimize(&dfa);
		dfa_free(&dfa);
	}

	nfa_free(&nfa);
}

static void join_dfa_blow(benchmark::State& state) {
	struct regexp_tree *re_tree;
	struct nfa nfa;
	struct dfa dfa_joined, dfa1, dfa2;

	re_tree = regexp_to_tree("/(a.*b|c.*d|e.*f|g.*h|j.*k|l.*m)/", NULL);

	nfa_alloc(&nfa);
	convert_tree_to_lambdanfa(&nfa, re_tree);
	regexp_tree_free(re_tree);
	nfa_rebuild(&nfa);

	dfa_alloc(&dfa1);
	convert_nfa_to_dfa(&dfa1, &nfa);
	nfa_free(&nfa);

	re_tree = regexp_to_tree("/(a.*b|c.*d|e.*f|g.*h|j.*k|l.*m)/", NULL);

	nfa_alloc(&nfa);
	convert_tree_to_lambdanfa(&nfa, re_tree);
	regexp_tree_free(re_tree);
	nfa_rebuild(&nfa);

	dfa_alloc(&dfa2);
	convert_nfa_to_dfa(&dfa2, &nfa);
	nfa_free(&nfa);

	for (auto _ : state) {
		dfa_alloc(&dfa_joined);
		dfa_join2(&dfa_joined, &dfa1, &dfa2);
		dfa_free(&dfa_joined);
	}

}

BENCHMARK(build_dfa_blow1);
BENCHMARK(build_dfa_blow1_minimize);
BENCHMARK(build_dfa_blow2);
BENCHMARK(build_dfa_blow2_minimize);
BENCHMARK(join_dfa_blow);

BENCHMARK_MAIN();
