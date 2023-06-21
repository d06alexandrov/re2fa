/*
 * Regular expressions parser.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include "parser.h"
#include "parser_inner.h"

#include <stdio.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define DMARK()		printf("[%s:%d]\n", __FILE__, __LINE__)

#define REGEXP_MAX_DEPTH	(240)

#define RE_CHARCLASS_INITIALIZER {.data = {0}, .inverse = false}

enum parser_error {
	PE_NONE			= 0,
	PE_UNKNOWN		= 1,
	PE_WRONG_SYNTAX		= 2,
	PE_NOT_IMPLEMENTED	= 3,
	PE_TOO_SHORT		= 4,
	PE_MALLOC		= 5,
	PE_NO_BEGIN		= 6,
	PE_NO_END		= 7,
	PE_TOO_DEEP		= 8,
	PE_NO_OPEN_BR		= 9,
	PE_NO_CLOSED_BR		= 10
};

const char	*parser_error_name_table[] = {
	[PE_NONE]		= "No error",
	[PE_UNKNOWN]		= "Unknown error",
	[PE_WRONG_SYNTAX]	= "Wrong syntax",
	[PE_NOT_IMPLEMENTED]	= "Something is not implemented... yet...",
	[PE_TOO_SHORT]		= "The pattern is too short",
	[PE_MALLOC]		= "Can not allocate memory",
	[PE_NO_BEGIN]		= "Pattern's begin not found",
	[PE_NO_END]		= "Pattern's end not found",
	[PE_TOO_DEEP]		= "Pattern is too deep",
	[PE_NO_OPEN_BR]		= "Non paired bracket",
	[PE_NO_CLOSED_BR]	= "Non paired bracket"
};

#define FP_FLAG_M	(0x0001)
#define FP_FLAG_S	(0x0002)
#define FP_FLAG_I	(0x0004)

struct first_pass_result {
	size_t			max_size;
	size_t			size;
	uint64_t		flags; /* s, m and i */
	struct first_pass_uchar	*parsed;
};

struct first_pass_uchar {
	uint8_t		depth;
	enum fp_type {
		FP_BEGIN_END	= 0,	/* begin or end of regular expression */
		FP_METACHAR	= 1,	/* ()[]. and etc. */
		FP_CHAR		= 2,	/* a, b, c ... even '\n' */
		FP_CHARSET	= 3,	/* \w, \d ...  */
		FP_CHARCLASS	= 4,
		FP_MINMAX	= 5
	} type;
	union {
		unsigned char		val;
		unsigned char		mc_val;
		unsigned char		cs_val;
		struct re_charclass	cc_val;
		struct {int min, max;}	mm_val;
	} data;
};

int first_pass_result_alloc(struct first_pass_result *dst, size_t size)
{
	memset(&(dst->flags), 0x00, sizeof(dst->flags));
	dst->max_size = size;
	dst->size = 0;

	if (size != 0)
		dst->parsed = malloc(sizeof(dst->parsed[0]) * size);
	else
		dst->parsed = NULL;

	return 0;
}

/* free() structure */
void first_pass_result_free(struct first_pass_result *ptr)
{
	ptr->size = 0;
	if (ptr->parsed != NULL) {
		free(ptr->parsed);
		ptr->parsed = NULL;
	}
}

void first_pass_result_add(struct first_pass_result *result,
			   struct first_pass_uchar element)
{
	void	*dst = &(result->parsed[result->size++]),
		*src = &element;
	memcpy(dst, src, sizeof(struct first_pass_uchar));
}

#define fp_result_last(result) (result->parsed[result->size - 1])
#define first_pass_result_last(result) (result->parsed[result->size - 1])
/* to decrease complexity introduced some macroses */
#define fp_result_add(...) first_pass_result_add(__VA_ARGS__)

struct second_pass_result {
	struct regexp_node	root;
};

/* ptr to first symbol after `\' */
int is_valid_octet(const unsigned char *ptr)
{
	if (ptr[0] >= '0' && ptr[0] <= '1' && ptr[1] >= '0' && ptr[1] <= '7'
					   && ptr[2] >= '0' && ptr[2] <= '7')
		return 1;
	else
		return 0;
}

unsigned char get_valid_octet_value(const unsigned char *ptr)
{
	return ((ptr[0] - 0x30) * 64 + (ptr[1] - 0x30) * 8 + (ptr[2] - 0x30));
}

/*
 * table's part (0x30-0x67)
 * equal to -1 if not hex character, otherwise equal to real value
 *
 */
const int8_t	hex_table_part[] = {
	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
	-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,10,11,12,13,14,15,-1
};
/* ptr to first symbol after `\' */
int is_valid_hex(const unsigned char *ptr)
{
	if (ptr[0] == 'x' && isxdigit(ptr[1]) && isxdigit(ptr[2]))
		return 1;
	else
		return 0;
}

unsigned char get_valid_hex_value(const unsigned char *ptr)
{
	unsigned char	c1 = (ptr[1] - 0x30),
			c2 = (ptr[2] - 0x30);
	return (hex_table_part[c1] * 16 + hex_table_part[c2]);
}

#define MINMAX_MAX_LEN	((sizeof(((struct first_pass_uchar *)0)->data.mm_val.max) * 8 * 3) / 10 + 1)

/*
 * {923,123}
 * ^ @pattern
 */
int parse_minmax(int *min, int *max, const unsigned char **pattern)
{
	enum parser_error	error = PE_NONE;
	int			cnt = 0;
	const unsigned char	*ptr;

	*min = *max = 0;
	ptr = *pattern;

	while (isdigit(*(++ptr)) && (cnt++) < MINMAX_MAX_LEN);

	*min = atoi((const char *)*pattern + 1);

	if (*ptr == '}') {
		*max = *min = atoi((const char *)*pattern + 1);
		goto out;
	} else if (*ptr != ',') {
		error = PE_WRONG_SYNTAX;
		goto out;
	}

	*pattern = ptr + 1;

	if (**pattern == '}') {
		*max = -1;
		ptr++;
		goto out;
	}


	while (isdigit(*(++ptr)) && (cnt++) < MINMAX_MAX_LEN);

	if (*ptr == '}') {
		*max = atoi((const char *)*pattern);
//		goto out;
	} else {
		error = PE_WRONG_SYNTAX;
		goto out;
	}

	if (*min > *max) {
		error = PE_WRONG_SYNTAX;
		goto out;
	}

out:
	*pattern = ptr;
	return error;
}

/*
 * NEED FIX! (error check is required) and must be simplier!
 * [abc]
 * ^   ^@pattern's position after return (if *error_ptr points to null)
 * ^@pattern points to pattern's begin (in function's start)
 */
int parse_charclass(struct re_charclass *result,
				  const unsigned char **pattern)
{
	enum parser_error	error = PE_UNKNOWN;
	const unsigned char	*ptr = *pattern + 1,
				*end = (unsigned char *)strrchr((char *)(*pattern) + 1, ']');
	unsigned char		val = 0x00;
	struct {int is_char; unsigned char val;} prev[2] = {{0, 0}, {0, 0}};
	*result = (struct re_charclass) RE_CHARCLASS_INITIALIZER;

	while (ptr < end) {
		switch (*ptr) {
		case ']':
			if (ptr != (unsigned char *)*pattern + 1 &&
				(!result->inverse || ptr != (unsigned char *)*pattern + 2))
				goto loop_exit;

			val = *ptr;
			goto simple_set;
			break;
		case '^':
			if (ptr == (unsigned char *)*pattern + 1)
				result->inverse = 1;
			else
				goto simple_set;
			break;
		case '-':
			prev[0] = prev[1];
			prev[1].is_char = 1;
			prev[1].val = '-';
			break;
		case '\\':
			switch (backslash_table_cc[*(++ptr)]) {
			case BS_HEX:
				if (is_valid_hex(ptr)) {
					val = get_valid_hex_value(ptr);
					ptr += 2;
					goto simple_set;
				}
				error = PE_UNKNOWN;
				goto error_out;
				break;
			case BS_OCTET:
				if (is_valid_octet(ptr)) {
					val = get_valid_octet_value(ptr);
					ptr += 2;
					goto simple_set;
				}
				error = PE_UNKNOWN;
				goto error_out;
				break;
			case BS_CHAR:
				val = backslash_replace_table_cc[*ptr];
				goto simple_set;
				break;
			case BS_CHARSET:
				set_charset_bits_cc(result->data, *ptr);
				break;
			case BS_UNKNOWN:
			default:
				error = PE_UNKNOWN;
				goto error_out;
				break;
			}

			break;
		default:
			val = *ptr;
		simple_set:
			if (prev[1].val == '-' && prev[1].is_char) {
				if (prev[0].is_char && prev[0].val <= val) {
					for (int i = prev[0].val; i < val; i++)
						SET_BIT(result->data, i);
				} else {
					SET_BIT(result->data, '-');
				}

				SET_BIT(result->data, val);
				prev[0].is_char = prev[1].is_char = 0;
				prev[0].val = prev[1].val = 0x00;
			} else {
				prev[0] = prev[1];
				prev[1].is_char = 1;
				prev[1].val = val;
				SET_BIT(result->data, val);
			}
		};

		ptr++;
	}
loop_exit:

	*pattern = ptr;

	return PE_NONE;
error_out:

	*pattern = ptr;

	return error;
}

int regexp_first_pass(struct first_pass_result *dst,
		      const char *regexp,
		      const char **error_ptr)
{
	int			error = PE_NONE;
	size_t			len = strlen(regexp);
	const unsigned char	*ptr = (unsigned char *)regexp,
				*end = (unsigned char *)strrchr(regexp, '/');
	struct first_pass_uchar	fpu_tmp, fpu_last;

	if (end == NULL || *(ptr++) != '/') {
		error = PE_NO_BEGIN;
		goto error_out_2;
	}

	if (ptr > end) {
		error = PE_NO_END;
		goto error_out_2;
	}

	if (first_pass_result_alloc(dst, len) != 0) {
		error = PE_MALLOC;
		goto error_out_2;
	}

	fpu_tmp.depth = 0;	/* depth */
	fpu_tmp.type = FP_BEGIN_END;
	first_pass_result_add(dst, fpu_tmp);

	fpu_tmp.depth++;

	while (ptr < end && fpu_tmp.depth <= REGEXP_MAX_DEPTH && fpu_tmp.depth > 0) {
		switch (*ptr) {
		case '/':
			/* multiple end */
			error = PE_WRONG_SYNTAX;
			goto error_out;
			break;
		case '\\':
			ptr++;
			switch (backslash_table_re[*ptr]) {
			case BS_CHAR:
				fpu_tmp.type = FP_CHAR;
				fpu_tmp.data.val = backslash_replace_table_re[*ptr];
				break;
			case BS_HEX:
				if (!is_valid_hex(ptr)) {
					error = PE_WRONG_SYNTAX;
					goto error_out;
				}
				fpu_tmp.type = FP_CHAR;
				fpu_tmp.data.val = get_valid_hex_value(ptr);
				ptr += 2;
				break;
			case BS_OCTET:
				if (!is_valid_octet(ptr)) {
					error = PE_WRONG_SYNTAX;
					goto error_out;
				}
				fpu_tmp.type = FP_CHAR;
				fpu_tmp.data.val = get_valid_octet_value(ptr);
				ptr += 2;
				break;
			case BS_CHARSET:
				fpu_tmp.type = FP_CHARSET;
				fpu_tmp.data.cs_val = *ptr;
				break;
			default:
				error = PE_WRONG_SYNTAX;
				goto error_out;
			}
			break;
		case '.':
		case '^':
		case '$':
		case '|':
			fpu_tmp.type = FP_METACHAR;
			fpu_tmp.data.mc_val = *ptr;
			break;
		case '*':
		case '+':
		case '?':
		case '{':
			fpu_last = first_pass_result_last(dst);
			if (fpu_last.type == FP_MINMAX &&
			    *ptr == '?')
				break;

			if (fpu_last.type == FP_BEGIN_END ||
			    (fpu_last.type == FP_METACHAR &&
                             fpu_last.data.mc_val != ')'  &&
                             fpu_last.data.mc_val != '.') ||
			    fpu_last.type == FP_MINMAX) {
				error = PE_WRONG_SYNTAX;
				goto error_out;
			}
			fpu_tmp.type = FP_MINMAX;
			if (*ptr == '{') {
				fpu_tmp.type = FP_MINMAX;
				error = parse_minmax(&fpu_tmp.data.mm_val.min,
						     &fpu_tmp.data.mm_val.max,
						     &ptr);
				if (error != PE_NONE)
					goto error_out;
			} else if (*ptr == '*') {
				fpu_tmp.data.mm_val.min = 0;
				fpu_tmp.data.mm_val.max = -1;
			} else if (*ptr == '+') {
				fpu_tmp.data.mm_val.min = 1;
				fpu_tmp.data.mm_val.max = -1;
			} else if (*ptr == '?') {
				fpu_tmp.data.mm_val.min = 0;
				fpu_tmp.data.mm_val.max = 1;
			}
			break;
		case '(':
			fpu_tmp.depth += (*ptr == '(' ? 1 : -1);
			fpu_tmp.type = FP_METACHAR;
			fpu_tmp.data.mc_val = *ptr;
			break;
		case ')':
			fpu_tmp.depth += (*ptr == '(' ? 1 : -1);
			fpu_tmp.type = FP_METACHAR;
			fpu_tmp.data.mc_val = *ptr;
			break;
		case '[':
			fpu_tmp.type = FP_CHARCLASS;
			error = parse_charclass(&(fpu_tmp.data.cc_val), &ptr);
			if (error != PE_NONE)
				goto error_out;
			break;
		case ']':
		case '}':
			error = PE_WRONG_SYNTAX;
			goto error_out;
			break;
		default:
			/* just a char */
			fpu_tmp.type = FP_CHAR;
			fpu_tmp.data.val = *ptr;
		}

		if (fpu_tmp.type == FP_METACHAR && *ptr == ')')
			fpu_tmp.depth--;
		first_pass_result_add(dst, fpu_tmp);
		if (fpu_tmp.type == FP_METACHAR && *ptr == '(')
			fpu_tmp.depth++;

		ptr++;
	}

	if (fpu_tmp.depth > REGEXP_MAX_DEPTH)
		error = PE_TOO_DEEP;
	else if (fpu_tmp.depth == 0)
		error = PE_NO_OPEN_BR;
	else if (fpu_tmp.depth != 1)
		error = PE_NO_CLOSED_BR;
	else if (ptr != end)
		error = PE_NO_END;

	if (error != PE_NONE)
		goto error_out;

	fpu_tmp.depth = 0;
	fpu_tmp.type = FP_BEGIN_END;
	first_pass_result_add(dst, fpu_tmp);

	while (*(++ptr) != '\0') {
		switch (*ptr) {
		case 'm':
			dst->flags |= FP_FLAG_M;
			break;
		case 's':
			dst->flags |= FP_FLAG_S;
			break;
		case 'i':
			dst->flags |= FP_FLAG_I;
			break;
		default:
//			error = PE_WRONG_SYNTAX;
//			goto error_out;
			break;
		}
	}

	return error;

error_out:
	first_pass_result_free(dst);
error_out_2:
	if (error_ptr != NULL)
		*error_ptr = (const char *)ptr;

	return -error;
}

int regexp_node_alloc(struct regexp_node *dst, int type)
{
	dst->type = type;
	dst->repeat.min = dst->repeat.max = 1;

	switch (type) {
	case RE_SPECIAL:
	case RE_EMPTY:
	case RE_CHAR:
	case RE_CHARCLASS:
		memset(&(dst->data), 0x00, sizeof(dst->data));
		break;
	case RE_CONCAT:
	case RE_UNION:
		dst->data.childs.cnt = 0;
		dst->data.childs.max_cnt = 0;
		dst->data.childs.ptr = NULL;
		break;
	};

	return 0;
}


/* replace it ↓ */
#define CHILD_COUNT	(512)

#define REGEXP_NODE_NEW(RE_NODE) \
		(&((RE_NODE)->data.childs.ptr[(RE_NODE)->data.childs.cnt++]))
#define REGEXP_NODE_LAST(RE_NODE) \
		(&((RE_NODE)->data.childs.ptr[(RE_NODE)->data.childs.cnt - 1]))

int regexp_node_add_child(struct regexp_node *dst, struct regexp_node **new)
{
	if (!(dst->type & (RE_CONCAT | RE_UNION)))
		return -1;

	if (dst->data.childs.cnt >= dst->data.childs.max_cnt) {
		dst->data.childs.max_cnt += 4;
		void	*tmp;
		tmp = realloc(dst->data.childs.ptr, sizeof(void *) * dst->data.childs.max_cnt);

		if (tmp == NULL)
			return -1;

		dst->data.childs.ptr = (struct regexp_node **) tmp;
	}

	dst->data.childs.ptr[dst->data.childs.cnt] = malloc(sizeof(struct regexp_node));

	if (dst->data.childs.ptr[dst->data.childs.cnt] == NULL)
		return -1;

	dst->data.childs.ptr[dst->data.childs.cnt]->parent = dst;

	if (new != NULL)
		*new = dst->data.childs.ptr[dst->data.childs.cnt];

	dst->data.childs.cnt++;

	return 0;
}

struct regexp_node *regexp_node_last_child(struct regexp_node *src)
{
	if (!(src->type & (RE_CONCAT | RE_UNION)) || src->data.childs.cnt == 0)
		return NULL;

	return src->data.childs.ptr[src->data.childs.cnt - 1];
}

int regexp_second_pass(struct second_pass_result *dst,
		       struct first_pass_result *src)
{
	int	error = PE_NONE;
	const struct first_pass_uchar	*ptr, *tmp_ptr;
	struct regexp_node	*cur_node = NULL, *tmp_node = &(dst->root);

	int	begin_symb = 0, end_symb = 0;

	for (ptr = src->parsed; (ptr - src->parsed) < src->size - 1; ptr++)
		switch (ptr->type) {
		case FP_BEGIN_END:
			goto regexp_begin;
		case FP_METACHAR:
			switch (ptr->data.mc_val) {
			case '(':
				regexp_node_add_child(cur_node, &tmp_node);
			regexp_begin:
				for (tmp_ptr = ptr + 1; tmp_ptr->depth > ptr->depth; tmp_ptr++)
					if (tmp_ptr->depth == ptr->depth + 1 &&
					    tmp_ptr->type == FP_METACHAR &&
					    tmp_ptr->data.mc_val == '|') {
						regexp_node_alloc(tmp_node, RE_UNION);
						cur_node = tmp_node;
						regexp_node_add_child(cur_node, &tmp_node);
						break;
					}
				regexp_node_alloc(tmp_node, RE_CONCAT);
				cur_node = tmp_node;
				break;
			case ')':
				cur_node = cur_node->parent;
				if (cur_node->type == RE_UNION)
					cur_node = cur_node->parent;
				break;
			case '.':
				regexp_node_add_child(cur_node, &tmp_node);
				regexp_node_alloc(tmp_node, RE_CHARCLASS);
				memset(tmp_node->data.cc_data.data, 0xFF, 256/8);
				if (!(src->flags & FP_FLAG_S))
					RST_BIT(tmp_node->data.cc_data.data, '\n');
				break;
			case '^':
				if (begin_symb != 0 || (ptr - src->parsed) > 1) {
					error = PE_NOT_IMPLEMENTED;
					goto error_out;
				} else {
					begin_symb = ptr - src->parsed;
				}
				break;
			case '$':
				if (end_symb != 0 || (ptr - src->parsed) != src->size - 2) {
					error = PE_NOT_IMPLEMENTED;
					goto error_out;
				} else {
					end_symb = ptr - src->parsed;
				}
				break;
			case '|':
				cur_node = cur_node->parent;
				regexp_node_add_child(cur_node, &tmp_node);
				regexp_node_alloc(tmp_node, RE_CONCAT);
				cur_node = tmp_node;
				break;
			}
			break;
		case FP_CHAR:
			if (isalpha(ptr->data.val) && (src->flags & FP_FLAG_I)) {
				regexp_node_add_child(cur_node, &tmp_node);
				regexp_node_alloc(tmp_node, RE_CHARCLASS);
				SET_BIT((tmp_node->data.cc_data.data),
					tolower(ptr->data.val));
				SET_BIT((tmp_node->data.cc_data.data),
					toupper(ptr->data.val));
			} else {
				regexp_node_add_child(cur_node, &tmp_node);
				regexp_node_alloc(tmp_node, RE_CHAR);
				tmp_node->data.c_val = ptr->data.val;
			}
			break;
		case FP_CHARSET:
			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CHARCLASS);
			set_charset_bits_re(tmp_node->data.cc_data.data,
					    ptr->data.cs_val);
			if (src->flags & FP_FLAG_I) {
				OR_BIT_GRP(tmp_node->data.cc_data.data,
					   0x41, 0x61, 26);
				OR_BIT_GRP(tmp_node->data.cc_data.data,
					   0x61, 0x41, 26);
			}
			break;
		case FP_CHARCLASS:
			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CHARCLASS);
			tmp_node->data.cc_data = ptr->data.cc_val;
			if (src->flags & FP_FLAG_I) {
				OR_BIT_GRP(tmp_node->data.cc_data.data,
					   0x41, 0x61, 26);
				OR_BIT_GRP(tmp_node->data.cc_data.data,
					   0x61, 0x41, 26);
			}
			break;
		case FP_MINMAX:
			tmp_node = regexp_node_last_child(cur_node);
			if (tmp_node == NULL) {
				error = PE_UNKNOWN;
				goto error_out;
			}
			tmp_node->repeat.min = ptr->data.mm_val.min;
			tmp_node->repeat.max = ptr->data.mm_val.max;
			break;
		}



	if (begin_symb == 0 || src->flags & FP_FLAG_M) {
		struct regexp_node	old_root = dst->root;
		cur_node = &dst->root;
		regexp_node_alloc(cur_node, RE_CONCAT);

		if (begin_symb == 0) {
			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CHARCLASS);
			tmp_node->repeat.min = 0;
			tmp_node->repeat.max = -1;
			tmp_node->data.cc_data.inverse = 0;
			memset(&tmp_node->data.cc_data.data, 0xFF,
			       sizeof(tmp_node->data.cc_data.data));
		} else {
			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_UNION);
			cur_node = tmp_node;

			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_EMPTY);

			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CONCAT);
			cur_node = tmp_node;

			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CHARCLASS);
			tmp_node->repeat.min = 0;
			tmp_node->repeat.max = -1;
			tmp_node->data.cc_data.inverse = 0;
			memset(&tmp_node->data.cc_data.data, 0xFF,
			       sizeof(tmp_node->data.cc_data.data));

			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CHAR);
			tmp_node->data.c_val = '\n';
		}

		regexp_node_add_child(&dst->root, &tmp_node);
		*tmp_node = old_root;
		tmp_node->parent = &dst->root;
	}

	if (end_symb == 0 || src->flags & FP_FLAG_M) {
		struct regexp_node	old_root = dst->root;
		regexp_node_alloc(&dst->root, RE_CONCAT);
		regexp_node_add_child(&dst->root, &tmp_node);
		*tmp_node = old_root;
		tmp_node->parent = &dst->root;

		cur_node = &dst->root;
		if (end_symb == 0) {
			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CHARCLASS);
			tmp_node->repeat.min = 0;
			tmp_node->repeat.max = -1;
			tmp_node->data.cc_data.inverse = 0;
			memset(&tmp_node->data.cc_data.data, 0xFF,
			       sizeof(tmp_node->data.cc_data.data));
		} else {
			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_UNION);
			cur_node = tmp_node;

			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_EMPTY);

			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CONCAT);
			cur_node = tmp_node;

			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CHAR);
			tmp_node->data.c_val = '\n';

			regexp_node_add_child(cur_node, &tmp_node);
			regexp_node_alloc(tmp_node, RE_CHARCLASS);
			tmp_node->repeat.min = 0;
			tmp_node->repeat.max = -1;
			tmp_node->data.cc_data.inverse = 0;
			memset(&tmp_node->data.cc_data.data, 0xFF,
			       sizeof(tmp_node->data.cc_data.data));
		}

	}



	return error;

error_out:

	return -error;
}

void regexp_node_free(struct regexp_node *re_node)
{
	if (re_node->type & (RE_CONCAT | RE_UNION)) {
		for (int i = 0; i < re_node->data.childs.cnt; i++) {
			regexp_node_free(re_node->data.childs.ptr[i]);
			free(re_node->data.childs.ptr[i]);
		}
		free(re_node->data.childs.ptr);
	}
}

void first_pass_result_print(struct first_pass_result *result,
				    const char *pattern,
				    const char *ptr);
void second_pass_result_print(struct second_pass_result *sp);

struct regexp_tree *regexp_to_tree(const char *regexp, const char **err)
{
	struct regexp_tree	*result = NULL;

	struct first_pass_result	fp;
	struct second_pass_result	sp;

	int	ret;

	const char *err_ptr;

	ret = regexp_first_pass(&fp, regexp, &err_ptr);

	if (ret != PE_NONE) {
		fprintf(stderr, "%s\n", parser_error_name_table[-ret]);
		fprintf(stderr, "%s\n", regexp);
		fprintf(stderr, "%*c\n", (int)(err_ptr - regexp), '^');
		return NULL;
	}

	ret = regexp_second_pass(&sp, &fp);

	first_pass_result_free(&fp);

	if (ret != PE_NONE) {
		fprintf(stderr, "%s\n", parser_error_name_table[-ret]);
		return 0;
	}

	result = malloc(sizeof(struct regexp_tree));

	result->root = sp.root;
	result->comment_size = strlen(regexp) + 1;
	result->comment = malloc(result->comment_size);
	memcpy(result->comment, regexp, result->comment_size);

	return result;
}

/*
 * frees allocated memory
 */
void regexp_tree_free(struct regexp_tree *re_tree)
{
	if (re_tree == NULL)
		return;
	regexp_node_free(&(re_tree->root));
	free(re_tree->comment);
	free(re_tree);
}



/* for debug */
#include <stdio.h>

void first_pass_result_print(struct first_pass_result *result,
			     const char *pattern,
			     const char *ptr)
{
	printf("fp_result:\n");
	printf("max_size = [%zu]; size=[%zu];""flags=[%016lX]\n",
		result->max_size, result->size, result->flags);

	for (size_t i = 0; i < result->size; i++) {
		switch(result->parsed[i].type) {
		case FP_BEGIN_END:
			printf("/");
			break;
		case FP_METACHAR:
			printf("mc	%c", result->parsed[i].data.mc_val);
			break;
		case FP_CHAR:
			printf("c	%c", result->parsed[i].data.val);
			break;
		case FP_CHARSET:
			printf("cs	%c", result->parsed[i].data.cs_val);
			break;
		case FP_CHARCLASS:
			printf("cc	[");
			for (unsigned int j = 0; j < 256; j++)
				if (isprint((unsigned char)j))
					if (GET_BIT(result->parsed[i].data.cc_val.data, j))
						printf("%c", (unsigned char)j);
			printf("]");
			break;
		case FP_MINMAX:
			printf("mm	{%d,%d}", result->parsed[i].data.mm_val.min
						, result->parsed[i].data.mm_val.max);
			break;
		}
		printf("\n");
	}


}

void regexp_node_print(struct regexp_node *);
void second_pass_result_print(struct second_pass_result *sp)
{
	struct regexp_node	*tmp;

	tmp = &(sp->root);

	regexp_node_print(tmp);
	printf("\n");
}

void regexp_node_print(struct regexp_node *node)
{
	switch (node->type) {
	case RE_SPECIAL:
		break;
	case RE_CHAR:
		printf("%c", node->data.c_val);
		break;
	case RE_CHARCLASS:
		printf("[");
			if (node->data.cc_data.inverse)
				printf("^ ");
			int	first = 0;
			int	cnt = 0;
			for (uint16_t i = 0; i < 256; i++)
				if (GET_BIT(node->data.cc_data.data, i))
					cnt++;

			if (cnt == 256)
				printf("0x00-0xFF");
			else
				for (uint16_t i = 0; i < 256; i++)
					if (GET_BIT(node->data.cc_data.data, i)) {
						if (first++) {
							printf(",");
						}
						if (isgraph((unsigned char) i))
							printf("%c", (unsigned char)i);
						else
							printf("0x%02X", (unsigned char)i);
					}
		printf("]");
		break;
	case RE_CONCAT:
		printf("(");
		if (node->data.childs.cnt > 0)
			regexp_node_print(node->data.childs.ptr[0]);
		for (int i = 1; i < node->data.childs.cnt; i++) {
			regexp_node_print(node->data.childs.ptr[i]);
		}
		printf(")");
		break;
	case RE_UNION:
		printf("(");
		if (node->data.childs.cnt > 0)
			regexp_node_print(node->data.childs.ptr[0]);
		for (int i = 1; i < node->data.childs.cnt; i++) {
			printf("|");
			regexp_node_print(node->data.childs.ptr[i]);
		}
		printf(")");
		break;
	case RE_EMPTY:
		break;
	}

	if (node->repeat.min != 1 || node->repeat.max != 1) {
		if (node->repeat.max != -1)
			printf("{%d,%d}", node->repeat.min, node->repeat.max);
		else
			printf("{%d,}", node->repeat.min);
	}

}

void regexp_tree_print(struct regexp_tree *re_tree)
{
	if (re_tree == NULL)
		return;

	regexp_node_print(&(re_tree->root));
	printf("\n");
}
