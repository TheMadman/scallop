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

#include "scallop/util.h"

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

void test_word()
{
	const char script[] = "foo";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);

	struct scallop_parse_token expected = {
		.token = SCALLOP_TOKEN_WORD,
		.start_offset = 0,
		.end_offset = 3,
	};
	struct scallop_parse_token actual = { 0 };
	actual = scallop_lex((csalt_store *)&csalt_script, actual);

	assert(expected.token == actual.token);
	assert(expected.start_offset == actual.start_offset);
	assert(expected.end_offset == actual.end_offset);

	actual = scallop_lex((csalt_store *)&csalt_script, actual);

	expected = (struct scallop_parse_token) {
		.token = SCALLOP_TOKEN_EOF,
		.start_offset = 3,
		.end_offset = 4,
	};

	assert(expected.token == actual.token);
	assert(expected.start_offset == actual.start_offset);
	assert(expected.end_offset == actual.end_offset);
}

void test_word_separator()
{
	const char script[] = " \t";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);

	struct scallop_parse_token expected = {
		.token = SCALLOP_TOKEN_WORD_SEPARATOR,
		.start_offset = 0,
		.end_offset = 2,
	};
	struct scallop_parse_token actual = { 0 };
	actual = scallop_lex((csalt_store *)&csalt_script, actual);

	assert(expected.token == actual.token);
	assert(expected.start_offset == actual.start_offset);
	assert(expected.end_offset == actual.end_offset);

	actual = scallop_lex((csalt_store *)&csalt_script, actual);

	expected = (struct scallop_parse_token) {
		.token = SCALLOP_TOKEN_EOF,
		.start_offset = 2,
		.end_offset = 3,
	};

	assert(expected.token == actual.token);
	assert(expected.start_offset == actual.start_offset);
	assert(expected.end_offset == actual.end_offset);
}

void test_short_phrase()
{
	const char script[] = "foo bar\tbaz";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);
	
	struct scallop_parse_token expected = {
		.token = SCALLOP_TOKEN_WORD,
		.start_offset = 0,
		.end_offset = 3,
	};
	struct scallop_parse_token actual = { 0 };
	actual = scallop_lex((csalt_store *)&csalt_script, actual);

	assert(expected.token == actual.token);
	assert(expected.start_offset == actual.start_offset);
	assert(expected.end_offset == actual.end_offset);
	
	actual = scallop_lex((csalt_store *)&csalt_script, actual);

	expected = (struct scallop_parse_token) {
		.token = SCALLOP_TOKEN_WORD_SEPARATOR,
		.start_offset = 3,
		.end_offset = 4,
	};

	assert(expected.token == actual.token);
	assert(expected.start_offset == actual.start_offset);
	assert(expected.end_offset == actual.end_offset);
}

void test_statements()
{
	const char script[] = "foo; bar baz\nbarry;";
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script);

	struct scallop_parse_token expected = {
		.token = SCALLOP_TOKEN_WORD,
		.start_offset = 0,
		.end_offset = 3,
	};
	struct scallop_parse_token actual = { 0 };
	actual = scallop_lex((csalt_store *)&csalt_script, actual);

	assert(expected.token == actual.token);
	assert(expected.start_offset == actual.start_offset);
	assert(expected.end_offset == actual.end_offset);
}

int main()
{
	test_word();
	test_word_separator();
	test_short_phrase();
//	test_statements();
//	test_quoted_strings();
//	test_double_quoted_strings();
//	test_open_curly_bracket();
	return EXIT_SUCCESS;
}
