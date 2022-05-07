/*
 * Copyright (C) <year>  Marcus Harrison
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "scallop/lexer.h"

#include <csalt/stores.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "scallop/scallop_util.h"

#define print_error(format, ...) fprintf(stderr, "%s:%d: " format "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)

const char *token_types[] = {
	"SCALLOP_TOKEN_WORD",
	"SCALLOP_TOKEN_WORD_SEPARATOR",
	"SCALLOP_TOKEN_STATEMENT_SEPARATOR",
	"SCALLOP_TOKEN_OPENING_CURLY_BRACKET",
	"SCALLOP_TOKEN_CLOSING_CURLY_BRACKET",
	"SCALLOP_TOKEN_OPENING_SQUARE_BRACKET",
	"SCALLOP_TOKEN_CLOSING_SQUARE_BRACKET",
	"SCALLOP_TOKEN_ASSIGNMENT_OPERATOR",
	"SCALLOP_TOKEN_PIPE",
	"SCALLOP_TOKEN_BINARY_PIPE",
};

void_fn *dummy_next(csalt_store *_, struct scallop_parse_token __, void *___)
{
	(void)_;
	(void)__;
	(void)___;
	return 0;
}

int test_word_result_called = 0;

void_fn *test_word_result(
	csalt_store *source,
	struct scallop_parse_token token,
	void *param
)
{
	(void)param;
	test_word_result_called = 1;
	char test_word_token_is_right = token.token == SCALLOP_TOKEN_WORD;
	char test_word_bounds_are_right = token.start_offset == 0 &&
		token.end_offset == 7;
	assert(test_word_token_is_right);
	assert(test_word_bounds_are_right);
	return (void_fn *)dummy_next;
}

int test_word()
{
	const char script[] = "w💩rd";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);
	int lex_result = scallop_lex(
		(csalt_store *)&csalt_script,
		test_word_result,
		0
	);
	assert(lex_result == 0);
	assert(test_word_result_called);
	return 0;
}

int test_word_separator_result_called = 0;

void_fn *test_word_separator_result(
	csalt_store *source,
	struct scallop_parse_token token,
	void *param
)
{
	(void)param;
	test_word_separator_result_called = 1;
	char test_word_separator_token_is_right =
		token.token == SCALLOP_TOKEN_WORD_SEPARATOR;
	char test_word_separator_bounds_are_right =
		token.start_offset == 0 &&
		token.end_offset == 2;
	assert(test_word_separator_token_is_right);
	assert(test_word_separator_bounds_are_right);
	return (void_fn *)dummy_next;
}

int test_word_separator()
{
	const char script[] = " \t";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);
	int lex_result = scallop_lex(
		(csalt_store *)&csalt_script,
		test_word_separator_result,
		0
	);
	assert(lex_result == 0);
	assert(test_word_separator_result_called);
	return 0;
}

int lex_short_phrase_result_called = 0;

void_fn *test_short_phrase_result(
	csalt_store *source,
	struct scallop_parse_token token,
	void *param
)
{
	(void)param;
	lex_short_phrase_result_called++;
	static int token_number = 0;
	struct scallop_parse_token expected = { 0 };
	switch (token_number++) {
		case 0:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 0;
			expected.end_offset = 3;
			break;
		case 1:
			expected.token = SCALLOP_TOKEN_WORD_SEPARATOR;
			expected.start_offset = 3;
			expected.end_offset = 4;
			break;
		case 2:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 4;
			expected.end_offset = 7;
			break;
		default:
			assert(!"Unexpected token");
	}

	int test_short_phrase_token_is_right =
		expected.token == token.token;
	int test_short_phrase_bounds_are_right =
		expected.start_offset == token.start_offset &&
		expected.end_offset == token.end_offset;
	assert(test_short_phrase_token_is_right);
	assert(test_short_phrase_bounds_are_right);
	return (void_fn *)test_short_phrase_result;
}

int test_short_phrase()
{
	const char script[] = "foo bar";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);
	int lex_result = scallop_lex(
		(csalt_store *)&csalt_script,
		test_short_phrase_result,
		0
	);
	assert(lex_short_phrase_result_called == 3);
	assert(lex_result == 0);
	return 0;
}

int test_statements_result_called = 0;

void_fn *test_statements_result(
	csalt_store *source,
	struct scallop_parse_token token,
	void *param
)
{
	(void)param;
	test_statements_result_called++;
	static int token_number = 0;
	struct scallop_parse_token expected;
	switch (token_number++) {
		case 0:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 0;
			expected.end_offset = 3;
			break;
		case 1:
			expected.token = SCALLOP_TOKEN_STATEMENT_SEPARATOR;
			expected.start_offset = 3;
			expected.end_offset = 5;
			break;
		case 2:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 5;
			expected.end_offset = 8;
			break;
		case 3:
			expected.token = SCALLOP_TOKEN_WORD_SEPARATOR;
			expected.start_offset = 8;
			expected.end_offset = 9;
			break;
		case 4:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 9;
			expected.end_offset = 12;
			break;
		case 5:
			expected.token = SCALLOP_TOKEN_STATEMENT_SEPARATOR;
			expected.start_offset = 12;
			expected.end_offset = 13;
			break;
		default:
			assert(!"Unexpected token");
	}

	int test_statements_token_is_same =
		expected.token == token.token;
	int test_statements_bounds_are_same =
		expected.start_offset == token.start_offset &&
		expected.end_offset == token.end_offset;
	assert(test_statements_token_is_same);
	assert(test_statements_bounds_are_same);
	return (void_fn *)test_statements_result;
}

int test_statements()
{
	const char script[] = "foo;\n"
		"bar baz;";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);
	int lex_result = scallop_lex(
		(csalt_store *)&csalt_script,
		test_statements_result,
		0
	);
	assert(test_statements_result_called == 6);
	assert(lex_result == 0);
	return 0;
}

int test_quoted_strings_result_called = 0;

void_fn *test_quoted_strings_result(
	csalt_store *source,
	struct scallop_parse_token result,
	void *param
)
{
	struct scallop_parse_token expected = { 0 };
	switch (test_quoted_strings_result_called++) {
		case 0:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 0;
			expected.end_offset = 11;
			break;
		case 1:
			expected.token = SCALLOP_TOKEN_WORD_SEPARATOR;
			expected.start_offset = 11;
			expected.end_offset = 12;
			break;
		case 2:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 12;
			expected.end_offset = 17;
			break;
		case 3:
			expected.token = SCALLOP_TOKEN_STATEMENT_SEPARATOR;
			expected.start_offset = 17;
			expected.end_offset = 18;
			break;
		case 4:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 18;
			expected.end_offset = 25;
			break;
		default:
			assert(!"Unexpected token");
	}

	char test_quoted_strings_token_is_same =
		result.token == expected.token;
	char test_quoted_strings_bounds_are_same =
		result.start_offset == expected.start_offset &&
		result.end_offset == expected.end_offset;

	assert(test_quoted_strings_token_is_same);
	assert(test_quoted_strings_bounds_are_same);
	return (void_fn *)test_quoted_strings_result;
}

int test_quoted_strings()
{
	const char script[] = "foo'bar'baz 'baz';'💩y'";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);
	int lex_result = scallop_lex(
		(csalt_store *)&csalt_script,
		test_quoted_strings_result,
		0
	);
	assert(test_quoted_strings_result_called == 5);
	assert(lex_result == 0);
	return 0;
}

int test_double_quoted_strings_result_called = 0;

void_fn *test_double_quoted_strings_result(
	csalt_store *source,
	struct scallop_parse_token result,
	void *param
)
{
	struct scallop_parse_token expected = { 0 };
	switch (test_double_quoted_strings_result_called++) {
		case 0:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 0;
			expected.end_offset = 11;
			break;
		case 1:
			expected.token = SCALLOP_TOKEN_WORD_SEPARATOR;
			expected.start_offset = 11;
			expected.end_offset = 12;
			break;
		case 2:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 12;
			expected.end_offset = 17;
			break;
		case 3:
			expected.token = SCALLOP_TOKEN_STATEMENT_SEPARATOR;
			expected.start_offset = 17;
			expected.end_offset = 18;
			break;
		case 4:
			expected.token = SCALLOP_TOKEN_WORD;
			expected.start_offset = 18;
			expected.end_offset = 25;
			break;
		default:
			assert(!"Unexpected token");
	}

	char test_quoted_strings_token_is_same =
		result.token == expected.token;
	char test_quoted_strings_bounds_are_same =
		result.start_offset == expected.start_offset &&
		result.end_offset == expected.end_offset;

	assert(test_quoted_strings_token_is_same);
	assert(test_quoted_strings_bounds_are_same);
	return (void_fn *)test_double_quoted_strings_result;

}

int test_double_quoted_strings()
{
	const char script[] = "foo\"bar\"baz \"baz\";\"💩y\"";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);
	int lex_result = scallop_lex(
		(csalt_store *)&csalt_script,
		test_double_quoted_strings_result,
		0
	);
	assert(test_double_quoted_strings_result_called == 5);
	assert(lex_result == 0);
	return 0;
}

int main()
{
	test_word();
	test_word_separator();
	test_short_phrase();
	test_statements();
	test_quoted_strings();
	test_double_quoted_strings();
	return EXIT_SUCCESS;
}
