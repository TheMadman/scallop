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

#define assert_tokens_equal(expected, actual) \
{ \
	assert((expected).token == (actual).token); \
	assert((expected).start_offset == (actual).start_offset); \
	assert((expected).end_offset == (actual).end_offset); \
}

const char *token_types[] = {
	"SCALLOP_TOKEN_EOF",
	"SCALLOP_TOKEN_WORD",
	"SCALLOP_TOKEN_WORD_SEPARATOR",
	"SCALLOP_TOKEN_STATEMENT_SEPARATOR",
	"SCALLOP_TOKEN_OPEN_CURLY_BRACKET",
	"SCALLOP_TOKEN_CLOSE_CURLY_BRACKET",
	"SCALLOP_TOKEN_OPEN_SQUARE_BRACKET",
	"SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET",
	"SCALLOP_TOKEN_ASSIGNMENT_OPERATOR",
	"SCALLOP_TOKEN_PIPE",
	"SCALLOP_TOKEN_BINARY_PIPE",
};

// This upsets the syntax highlighter of (n)vim.
// Not gonna lie, it kinda upsets me too.
#define expect(script, ...) \
{ \
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script); \
	csalt_store * const store = (csalt_store *)&csalt_script; \
	static const struct scallop_parse_token expects[] = { \
		__VA_ARGS__ \
	}; \
	const struct scallop_parse_token *expected = expects; \
	struct scallop_parse_token actual = { 0 }; \
	for ( \
		actual = scallop_lex(store, actual); \
		actual.token != SCALLOP_TOKEN_EOF; \
		actual = scallop_lex(store, actual), ++expected \
	) { \
		print_error( \
			"expected: %s %ld -> %ld", \
			token_types[expected->token], \
			expected->start_offset, \
			expected->end_offset \
		); \
		print_error( \
			"actual: %s %ld -> %ld", \
			token_types[actual.token], \
			actual.start_offset, \
			actual.end_offset \
		); \
		assert_tokens_equal(*expected, actual); \
	} \
}

void test_word()
{
	static const char script[] = "foo";

	expect(script,
		{ SCALLOP_TOKEN_WORD, 0, 3 },
		{ SCALLOP_TOKEN_EOF, 3, 4 }
	);
}

void test_word_separator()
{
	static const char script[] = " \t";
	
	expect(script,
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 0, 2 },
		{ SCALLOP_TOKEN_EOF, 2, 3 }
	);
}

void test_short_phrase()
{
	static const char script[] = "foo bar\tbaz";

	expect(script,
		{ SCALLOP_TOKEN_WORD, 0, 3 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 3, 4 },
		{ SCALLOP_TOKEN_WORD, 4, 7 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 7, 8 },
		{ SCALLOP_TOKEN_WORD, 8, 11 },
		{ SCALLOP_TOKEN_EOF, 11, 12 }
	);
}

void test_statements()
{
	static const char script[] = "foo; bar baz\nbarry;";

	expect(script,
		{ SCALLOP_TOKEN_WORD, 0, 3 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 3, 4 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 4, 5 },
		{ SCALLOP_TOKEN_WORD, 5, 8 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 8, 9 },
		{ SCALLOP_TOKEN_WORD, 9, 12 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 12, 13 },
		{ SCALLOP_TOKEN_WORD, 13, 18 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 18, 19 },
		{ SCALLOP_TOKEN_EOF, 19, 20 }
	);
}

void test_quoted_strings()
{
	static const char script[] = "'foo' foo'bar' 'bar'baz foo'bar'baz";

	expect(script,
		{ SCALLOP_TOKEN_WORD, 0, 5 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 5, 6 },
		{ SCALLOP_TOKEN_WORD, 6, 14},
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 14, 15 },
		{ SCALLOP_TOKEN_WORD, 15, 23 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 23, 24 },
		{ SCALLOP_TOKEN_WORD, 24, 35 },
		{ SCALLOP_TOKEN_EOF, 35, 36 }
	);
}

void test_double_quoted_strings()
{
	static const char script[] = "\"foo\" foo\"bar\" \"bar\"baz foo\"bar\"baz";

	expect(script,
		{ SCALLOP_TOKEN_WORD, 0, 5 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 5, 6 },
		{ SCALLOP_TOKEN_WORD, 6, 14},
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 14, 15 },
		{ SCALLOP_TOKEN_WORD, 15, 23 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 23, 24 },
		{ SCALLOP_TOKEN_WORD, 24, 35 },
		{ SCALLOP_TOKEN_EOF, 35, 36 }
	);
}

void test_open_curly_brackets()
{
	static const char script[] = "{ {a{{💩{;{\"{\"{'{'";
	expect(script,
		{ SCALLOP_TOKEN_OPEN_CURLY_BRACKET, 0, 1 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 1, 2 },
		{ SCALLOP_TOKEN_OPEN_CURLY_BRACKET, 2, 3 },
		{ SCALLOP_TOKEN_WORD, 3, 4 },
		{ SCALLOP_TOKEN_OPEN_CURLY_BRACKET, 4, 5 },
		{ SCALLOP_TOKEN_OPEN_CURLY_BRACKET, 5, 6 },
		{ SCALLOP_TOKEN_WORD, 6, 10 },
		{ SCALLOP_TOKEN_OPEN_CURLY_BRACKET, 10, 11 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 11, 12 },
		{ SCALLOP_TOKEN_OPEN_CURLY_BRACKET, 12, 13 },
		{ SCALLOP_TOKEN_WORD, 13, 16 },
		{ SCALLOP_TOKEN_OPEN_CURLY_BRACKET, 16, 17 },
		{ SCALLOP_TOKEN_EOF, 6, 7 }
	);
}

void test_close_curly_brackets()
{
	static const char script[] = "} }a}}💩};}\"}\"}'}'";
	expect(script,
		{ SCALLOP_TOKEN_CLOSE_CURLY_BRACKET, 0, 1 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 1, 2 },
		{ SCALLOP_TOKEN_CLOSE_CURLY_BRACKET, 2, 3 },
		{ SCALLOP_TOKEN_WORD, 3, 4 },
		{ SCALLOP_TOKEN_CLOSE_CURLY_BRACKET, 4, 5 },
		{ SCALLOP_TOKEN_CLOSE_CURLY_BRACKET, 5, 6 },
		{ SCALLOP_TOKEN_WORD, 6, 10 },
		{ SCALLOP_TOKEN_CLOSE_CURLY_BRACKET, 10, 11 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 11, 12 },
		{ SCALLOP_TOKEN_CLOSE_CURLY_BRACKET, 12, 13 },
		{ SCALLOP_TOKEN_WORD, 13, 16 },
		{ SCALLOP_TOKEN_CLOSE_CURLY_BRACKET, 16, 17 },
		{ SCALLOP_TOKEN_EOF, 6, 7 }
	);
}

void test_open_square_brackets()
{
	static const char script[] = "[ [a[[💩[;[\"[\"['['";
	expect(script,
		{ SCALLOP_TOKEN_OPEN_SQUARE_BRACKET, 0, 1 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 1, 2 },
		{ SCALLOP_TOKEN_OPEN_SQUARE_BRACKET, 2, 3 },
		{ SCALLOP_TOKEN_WORD, 3, 4 },
		{ SCALLOP_TOKEN_OPEN_SQUARE_BRACKET, 4, 5 },
		{ SCALLOP_TOKEN_OPEN_SQUARE_BRACKET, 5, 6 },
		{ SCALLOP_TOKEN_WORD, 6, 10 },
		{ SCALLOP_TOKEN_OPEN_SQUARE_BRACKET, 10, 11 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 11, 12 },
		{ SCALLOP_TOKEN_OPEN_SQUARE_BRACKET, 12, 13 },
		{ SCALLOP_TOKEN_WORD, 13, 16 },
		{ SCALLOP_TOKEN_OPEN_SQUARE_BRACKET, 16, 17 },
		{ SCALLOP_TOKEN_EOF, 6, 7 }
	);
}

void test_close_square_brackets()
{
	static const char script[] = "] ]a]]💩];]\"]\"]']'";
	expect(script,
		{ SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET, 0, 1 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 1, 2 },
		{ SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET, 2, 3 },
		{ SCALLOP_TOKEN_WORD, 3, 4 },
		{ SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET, 4, 5 },
		{ SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET, 5, 6 },
		{ SCALLOP_TOKEN_WORD, 6, 10 },
		{ SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET, 10, 11 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 11, 12 },
		{ SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET, 12, 13 },
		{ SCALLOP_TOKEN_WORD, 13, 16 },
		{ SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET, 16, 17 },
		{ SCALLOP_TOKEN_EOF, 6, 7 }
	);
}

void test_escape_unquoted()
{
	static const char script[] = "\\\"a\\a \\z;\\b";
	expect(script,
		{ SCALLOP_TOKEN_WORD, 0, 5 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 5, 6 },
		{ SCALLOP_TOKEN_WORD, 6, 8 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 8, 9 },
		{ SCALLOP_TOKEN_WORD, 9, 11 },
		{ SCALLOP_TOKEN_EOF, 11, 12 }
	);
}

int main()
{
	test_word();
	test_word_separator();
	test_short_phrase();
	test_statements();
	test_quoted_strings();
	test_double_quoted_strings();
	test_open_curly_brackets();
	test_close_curly_brackets();
	test_open_square_brackets();
	test_close_square_brackets();
	test_escape_unquoted();
	return EXIT_SUCCESS;
}
