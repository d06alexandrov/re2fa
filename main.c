/*
 * Main application for manipulations with regular expressions
 * and finite-state automata.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#include <pthread.h>

#include "lib/dfa.h"
#include "lib/nfa.h"
#include "lib/tree_to_nfa.h"
#include "lib/nfa_to_dfa.h"

#include "version_info.h"

const char *argp_program_version = NULL;
const char *argp_program_bug_address = "d06alexandrov@gmail.com";

static char doc[] =
		"A program to manipulate with regexp and fa.\n"
		"Use it carefully.\n";

static char args_doc[] = "ARG...";

#define OPT_I_TYPE	1
#define OPT_O_TYPE	2

static struct argp_option options[] = {
	{"input-type",	OPT_I_TYPE,	"TYPE",	0, "Input type", 0},
	{"output",	'o',		"FILE",	0, "Save output to FILE", 0},
	{"output-type", OPT_O_TYPE,	"TYPE",	0, "Output type", 0},
	{"threads",	't',		"TNUM", 0, "Number of threads", 1},
	{"verbose",	'v',		0,	0, "Verbose output", 2},
	{"join",	'j',		0,	0, "Join inputs into one output", 1},
	{"minimize",	'm',		0,	0, "Minimize automaton", 1},
	{0}
};

struct arguments {
	char	**input;
	int	input_type;
	int	input_cnt;

	char	*output_path;
	int	output_type;

	int	verbose;
	int	join;
	int	minimize;

	int	thread_cnt;
};

struct arguments	arguments;

#define FAT_REGEXP	1
#define FAT_REGEXP_FILE	2
#define FAT_NFA_FILE	4
#define FAT_DFA_FILE	6

struct fa_type {
	const char	*name;
	int		id;
	const char	*desc;
};

static struct fa_type fa_types[] = {
	{"regexp",	FAT_REGEXP,		0},
	{"regexp-file",	FAT_REGEXP_FILE,	0},
	{"dfa-file",	FAT_DFA_FILE,		0},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments	*args = state->input;

	switch (key) {
	case OPT_I_TYPE:
	case OPT_O_TYPE:
		for (struct fa_type *ptr = fa_types; ptr->name != NULL; ptr++)
			if (strcmp(ptr->name, arg) == 0) {
				if (key == OPT_I_TYPE)
					args->input_type = ptr->id;
				else
					args->output_type = ptr->id;
				return 0;
			}
		fprintf(stderr, "Unknown type: %s\n", arg);
		break;
	case 'j':
		args->join = 1;
		break;
	case 'm':
		args->minimize = 1;
		break;
	case 'o':
		args->output_path = arg;
		break;
	case 't':
		args->thread_cnt = atoi(arg);
		if (args->thread_cnt < 1)
			return ARGP_ERR_UNKNOWN;
		break;
	case 'v':
		args->verbose = 1;
		break;
	case ARGP_KEY_ARG:
		args->input = realloc(args->input,
				      sizeof(char *) * (args->input_cnt + 1));
		args->input[args->input_cnt++] = arg;
		break;
	case ARGP_KEY_ERROR:
		if (args->input_cnt != 0)
			free(args->input);
		break;
	default:
		break;
	}

	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

int main_regexp_to_nfa(struct nfa **, char **, int *);
int main_nfa_to_dfa(struct dfa **, struct nfa *, int *cnt);
/* join dfa into one */
int main_dfa_join(struct dfa *dfa, int cnt, int t_cnt);

int main(int argc, char **argv)
{
	argp_program_version = git_version;
	int	ret = 0;
	arguments.input		= NULL;
	arguments.input_type	= 0;
	arguments.input_cnt	= 0;
	arguments.output_path	= NULL;
	arguments.output_type	= FAT_DFA_FILE;
	arguments.verbose	= 0;
	arguments.join		= 0;
	arguments.minimize	= 0;
	arguments.thread_cnt	= 1;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if (arguments.input_cnt == 0 || arguments.input_type == 0) {
		ret = -1;
		goto out;
	}


	int	regexp_cnt = 0, nfa_cnt = 0, dfa_cnt = 0;
	char		**regexp = NULL;
	struct nfa	*nfa = NULL;
	struct dfa	*dfa = NULL;

	switch (arguments.input_type) {
	case FAT_REGEXP:
	case FAT_REGEXP_FILE:
		if (arguments.input_type == FAT_REGEXP) {
			regexp_cnt = arguments.input_cnt;
			regexp = malloc(sizeof(char *) * arguments.input_cnt);
			for (int i = 0; i < arguments.input_cnt; i++) {
				regexp[i] = malloc(strlen(arguments.input[i]) + 1);
				memcpy(regexp[i], arguments.input[i],
				       strlen(arguments.input[i]) + 1);
			}
		} else {
			regexp_cnt = 0;
			for (int i = 0; i < arguments.input_cnt; i++) {
#define REGEXP_BUFFER_SIZE	(1024 * 16)
				FILE	*file;
				char	buffer[REGEXP_BUFFER_SIZE];
				file = fopen(arguments.input[i], "r");
				if (file == NULL)
					continue;
				while (fgets(buffer, REGEXP_BUFFER_SIZE, file) != NULL) {
					if (strlen(buffer) == REGEXP_BUFFER_SIZE - 1) {
						/* bad */
					}
					regexp = realloc(regexp, sizeof(char *) * (regexp_cnt + 1));
					regexp[regexp_cnt] = malloc(strlen(buffer) + 1);
					memcpy(regexp[regexp_cnt], buffer,
					       strlen(buffer) + 1);
					regexp_cnt++;
				}
				fclose(file);
			}
		}
		if (regexp_cnt == 0)
			return -1;
	case FAT_NFA_FILE:
		if (arguments.input_type == FAT_NFA_FILE) {
		} else {
			nfa_cnt = regexp_cnt;
			main_regexp_to_nfa(&nfa, regexp, &nfa_cnt);
			for (int i = 0; i < regexp_cnt; i++)
				free(regexp[i]);
			free(regexp);
			if (nfa_cnt == 0)
				return -1;
		}
	case FAT_DFA_FILE:
		if (arguments.input_type == FAT_DFA_FILE) {
			dfa_cnt = 0;
			dfa = malloc(sizeof(struct dfa) * arguments.input_cnt);
			for (int i = 0; i < arguments.input_cnt; i++) {
				if (dfa_load_from_file(&dfa[dfa_cnt], arguments.input[i]) != 0) {
					fprintf(stderr, "Bad file %s\n", arguments.input[i]);
					continue;
				} else {
					dfa_cnt++;
				}
			}
		} else {
			dfa_cnt = nfa_cnt;
			main_nfa_to_dfa(&dfa, nfa, &dfa_cnt);
			for (int i = 0; i < nfa_cnt; i++)
				nfa_free(nfa + i);
			free(nfa);
		}

		if (dfa_cnt == 0)
			return -1;

		if (arguments.minimize)
			for (int i = 0; i < dfa_cnt; i++) {
				size_t	before = dfa[i].state_cnt;
				dfa_minimize(&dfa[i]);
				if (arguments.verbose) {
					printf("dfa minimized %zu->%zu\n",
					       before,
					       dfa[i].state_cnt);
				}
			}


		break;
	default:
		return -1;
	}


	if (arguments.verbose)
		for (int j = 0; j < dfa_cnt; j++) {
			printf("[dfa]\n"
			       "  state cnt: %zu, bps: %d\n",
				dfa[j].state_cnt, dfa[j].bps);
		}

	if (dfa_cnt > 1 && arguments.join) {
		main_dfa_join(dfa, dfa_cnt, arguments.thread_cnt);

		if (arguments.verbose)
			printf("joined dfa %zu\n", dfa->state_cnt);

		if (arguments.output_path != NULL) {
			dfa_save_to_file(dfa, arguments.output_path);
		}

		dfa_free(dfa);
		free(dfa);
	} else {

		if (dfa_cnt == 1 && arguments.output_path != NULL)
			dfa_save_to_file(dfa, arguments.output_path);
		for (int j = 0; j < dfa_cnt; j++)
			dfa_free(&dfa[j]);
		free(dfa);
	}

out:
	if (arguments.input_cnt != 0)
		free(arguments.input);

	return ret;
}

int main_regexp_to_nfa(struct nfa **nfa, char **regexp, int *cnt)
{
	int	processed = 0;
	*nfa = malloc(sizeof(struct nfa) * (*cnt));

	for (int i = 0; i < *cnt; i++) {
		struct regexp_tree	*tree;
		tree = regexp_to_tree(regexp[i], NULL);
		if (tree == NULL)
			continue;
		nfa_alloc(&(*nfa)[processed]);
		convert_tree_to_lambdanfa(&(*nfa)[processed], tree);
		regexp_tree_free(tree);
		processed++;
	}

	*nfa = realloc(*nfa, sizeof(struct nfa) * processed);

	*cnt = processed;

	return 0;
}

int main_nfa_to_dfa(struct dfa **dfa, struct nfa *nfa, int *cnt)
{
	int	processed = 0;
	*dfa = malloc(sizeof(struct dfa) * *cnt);

	for (int i = 0; i < *cnt; i++) {
		nfa_rebuild(&nfa[i]);
		dfa_alloc(&(*dfa)[i]);
		convert_nfa_to_dfa(&(*dfa)[i], &nfa[i]);
		if (arguments.minimize)
			dfa_minimize(&(*dfa)[i]);
		processed++;
	}

	*dfa = realloc(*dfa, sizeof(struct dfa) * processed);

	*cnt = processed;

	return 0;
}

struct thread_task_join {
	pthread_t	thread_id;

	int		t_id, t_cnt;
	int		*t_states; /* int[t_cnt], stores thread info */

	int		dfa_cnt; /* with first 't_cnt' dfa */
	struct dfa	*dfa;
};

void *thread_to_join(void *);

int main_dfa_join(struct dfa *dfa, int cnt, int t_cnt)
{
	if (cnt == 1)
		return 0;

	if (cnt / 2 < t_cnt)
		t_cnt = cnt / 2;

	struct thread_task_join	*tasks;
	int	*t_states = malloc(sizeof(int) * t_cnt);
	tasks = malloc(sizeof(struct thread_task_join) * t_cnt);
	for (int i = 0; i < t_cnt; i++) {
		t_states[i] = 0;
		tasks[i].t_id = i;
		tasks[i].t_cnt = t_cnt;
		tasks[i].t_states = t_states;
		tasks[i].dfa_cnt = cnt;
		tasks[i].dfa = dfa;
	}

	for (int i = 0; i < t_cnt; i++)
		pthread_create(&tasks[i].thread_id, NULL,
			       thread_to_join, &tasks[i]);

	for (int i = 0; i < t_cnt; i++)
		pthread_join(tasks[i].thread_id, NULL);

	free(tasks);
	free(t_states);

	return 0;
}

void *thread_to_join(void *arg)
{
	struct thread_task_join	*task = arg;
	static pthread_mutex_t	join_lock = PTHREAD_MUTEX_INITIALIZER;

	static int	joined = 0;

	for (;;) {
		struct dfa	*dfa = NULL;
		pthread_mutex_lock(&join_lock);

		if (joined + task->t_cnt < task->dfa_cnt) {
			dfa = &task->dfa[task->t_cnt + joined++];
		} else {
			if (joined == task->dfa_cnt - 1) {
				if (task->t_id != 0)
					task->dfa[0] = task->dfa[task->t_id];
			} else {
				for (int i = 0; i < task->t_cnt; i++)
					if (task->t_states[i] == 1) {
						dfa = &task->dfa[i];
						task->t_states[i] = 2;
						joined++;
					}
				if (dfa == NULL)
					task->t_states[task->t_id] = 1;
			}
		}

		pthread_mutex_unlock(&join_lock);

		if (dfa == NULL)
			break;

		dfa_join(&task->dfa[task->t_id], dfa);
		dfa_free(dfa);

		size_t	size = task->dfa[task->t_id].state_cnt;
		if (arguments.minimize)
			dfa_minimize(&task->dfa[task->t_id]);
		dfa_compress(&task->dfa[task->t_id]);
		fprintf(stderr, "[tid:%d] joined and minimized %zu->%zu\n",
				 task->t_id,
				 size,
				 task->dfa[task->t_id].state_cnt);
		fprintf(stderr, "bps[%d], state_size[%zu], state_max_cnt[%zu]\n", task->dfa[task->t_id].bps, task->dfa[task->t_id].state_size, task->dfa[task->t_id].state_max_cnt);
	}
	fprintf(stderr, "[tid:%d] finished \n", task->t_id);

	return NULL;
}
