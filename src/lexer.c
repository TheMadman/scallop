/*
 * Scallop - a shell for executing tasks concurrently
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

#include <stdio.h> // EOF
#include <stdlib.h>
#include <csalt/resources.h>
#include <assert.h>

// This is all probably horribly inefficient but whatever

static int get_char_internal(csalt_store *store, void *param)
{
	return !!csalt_store_read(store, param, 1);
}

static char get_char(csalt_store *store, ssize_t index)
{
	char result = 0;
	int success = csalt_store_split(store, index, index + 1, get_char_internal, &result);
	if (!success)
		return 0;
	return result;
}

enum CHAR_TYPE {
	CHAR_NULL,
	CHAR_ASCII_PRINTABLE,
	CHAR_UTF8_START,
	CHAR_UTF8_CONT,
	CHAR_OPEN_CURLY_BRACKET,
	CHAR_CLOSE_CURLY_BRACKET,
	CHAR_OPEN_SQUARE_BRACKET,
	CHAR_CLOSE_SQUARE_BRACKET,
	CHAR_QUOTE,
	CHAR_DOUBLE_QUOTE,
	CHAR_BACKSLASH,
	CHAR_WORD_SEPARATOR,
	CHAR_STATEMENT_SEPARATOR,
	CHAR_PIPE,
	CHAR_UNKNOWN,
};

#define UTF8_FIRST_BIT (1 << 7)
#define UTF8_SECOND_BIT (1 << 6)

static enum CHAR_TYPE char_type(char character)
{
	switch (character) {
		case '{':
			return CHAR_OPEN_CURLY_BRACKET;
		case '}':
			return CHAR_CLOSE_CURLY_BRACKET;
		case '[':
			return CHAR_OPEN_SQUARE_BRACKET;
		case ']':
			return CHAR_CLOSE_SQUARE_BRACKET;
		case '\'':
			return CHAR_QUOTE;
		case '"':
			return CHAR_DOUBLE_QUOTE;
		case '\\':
			return CHAR_BACKSLASH;
		case ';':
		case '\n':
			return CHAR_STATEMENT_SEPARATOR;
		case ' ':
		case '\t':
			return CHAR_WORD_SEPARATOR;
		case '|':
			return CHAR_PIPE;
		case '\0':
			return CHAR_NULL;
	}

	if (' ' <= character && character <= '~')
		return CHAR_ASCII_PRINTABLE;

	unsigned char utf8_bits = UTF8_FIRST_BIT | UTF8_SECOND_BIT;
	switch (character & utf8_bits) {
		// UTF-8 continuation character
		case UTF8_FIRST_BIT:
			return CHAR_UTF8_CONT;
		// UTF-8 start character
		case UTF8_FIRST_BIT | UTF8_SECOND_BIT:
			return CHAR_UTF8_START;
	}

	return CHAR_UNKNOWN;
}

struct next_char {
	enum CHAR_TYPE type;
	struct scallop_parse_token token;
};

static struct next_char next_char(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	char c = get_char(store, ++token.end_offset);
	char is_newline = c == '\n';
	enum CHAR_TYPE type = char_type(c);
	char is_utf8_cont = type == CHAR_UTF8_CONT;
	token.row += is_newline;
	token.col = is_newline ? 1 :
		is_utf8_cont? token.col: token.col + 1;
	return (struct next_char) {
		type,
		token,
	};
}

typedef struct scallop_parse_token lex_fn(
	csalt_store *,
	struct scallop_parse_token
);

struct scallop_parse_token lex_error(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	(void)store;
	(void)token;
	assert(!"`lex_fn *lex_error` should never be called!");
	return (struct scallop_parse_token) {
		.token = SCALLOP_TOKEN_EOF,
		.start_offset = -1,
		.end_offset = -1,
	};
}

/*
 * State-transition-table-lite
 *
 * While this struct and function might let
 * us use one large state transition table
 * for all transitions (and only need one
 * function), it's a slow run-time linear
 * search, so I've separated out the "initial
 * states" into separate functions.
 *
 * It's still a linear search at run-time,
 * but only for the states we know start from
 * the current state, instead of all states.
 *
 * Of course, C++ gives you constexpr, which
 * is the _correct_ way to solve this problem...
 * but I don't fancy going through macros or
 * code generation to do this "properly" in C.
 */
struct state_transition_row {
	enum CHAR_TYPE input;
	lex_fn *new_state;
};

static lex_fn *transition_state_bounds(
	const struct state_transition_row *rows_begin,
	const struct state_transition_row *rows_end,
	const enum CHAR_TYPE input
)
{
	for (
		const struct state_transition_row *current = rows_begin;
		current < rows_end;
		current++
	) {
		if (input == current->input) {
			return current->new_state;
		}
	}
	return lex_error;
}

#define transition_state(array, input) \
	transition_state_bounds((array), arrend(array), input)

struct scallop_parse_token lex_end(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_eof(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_utf8_start(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_utf8_cont(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_quoted_utf8_start(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_quoted_utf8_cont(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_word_separator(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_statement_separator(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_quoted_string(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_end_quoted_string(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_double_quoted_utf8_start(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_double_quoted_utf8_cont(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_double_quoted_string(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_end_double_quoted_string(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_word(
	csalt_store *,
	struct scallop_parse_token
);
struct scallop_parse_token lex_begin(
	csalt_store *,
	struct scallop_parse_token
);

/*
 * Whichever token we're currently lexing, return it
 */
struct scallop_parse_token lex_end(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	return token;
}

struct scallop_parse_token lex_eof(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	token.token = SCALLOP_TOKEN_EOF;
	++token.end_offset;
	return token;
}

struct scallop_parse_token lex_utf8_start(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	static const struct state_transition_row transitions[] = {
		{ CHAR_UTF8_CONT, lex_utf8_cont },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_utf8_cont(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	static const struct state_transition_row transitions[] = {
		{ CHAR_UTF8_START, lex_utf8_start },
		{ CHAR_UTF8_CONT, lex_utf8_cont },
		{ CHAR_ASCII_PRINTABLE, lex_word },
		{ CHAR_WORD_SEPARATOR, lex_end },
		{ CHAR_STATEMENT_SEPARATOR, lex_end },
		{ CHAR_NULL, lex_end },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_quoted_utf8_start(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	static const struct state_transition_row transitions[] = {
		{ CHAR_UTF8_CONT, lex_quoted_utf8_cont },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_quoted_utf8_cont(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	static const struct state_transition_row transitions[] = {
		{ CHAR_UTF8_CONT, lex_quoted_utf8_cont },
		{ CHAR_ASCII_PRINTABLE, lex_quoted_string },
		{ CHAR_WORD_SEPARATOR, lex_quoted_string },
		{ CHAR_STATEMENT_SEPARATOR, lex_quoted_string },
		{ CHAR_DOUBLE_QUOTE, lex_quoted_string },
		{ CHAR_WORD_SEPARATOR, lex_quoted_string },
		{ CHAR_QUOTE, lex_end },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_quoted_string(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	token.token = SCALLOP_TOKEN_WORD;
	static const struct state_transition_row transitions[] = {
		{ CHAR_ASCII_PRINTABLE, lex_quoted_string },
		{ CHAR_UTF8_START, lex_quoted_utf8_start },
		{ CHAR_WORD_SEPARATOR, lex_quoted_string },
		{ CHAR_STATEMENT_SEPARATOR, lex_quoted_string },
		// { CHAR_BACKSLASH, lex_escape },
		{ CHAR_DOUBLE_QUOTE, lex_double_quoted_string },
		{ CHAR_QUOTE, lex_end_quoted_string },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_end_quoted_string(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	static const struct state_transition_row transitions[] = {
		{ CHAR_WORD_SEPARATOR, lex_end },
		{ CHAR_STATEMENT_SEPARATOR, lex_end },
		{ CHAR_ASCII_PRINTABLE, lex_word },
		{ CHAR_UTF8_START, lex_utf8_start },
		{ CHAR_QUOTE, lex_quoted_string },
		{ CHAR_DOUBLE_QUOTE, lex_double_quoted_string },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_double_quoted_utf8_start(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	static const struct state_transition_row transitions[] = {
		{ CHAR_UTF8_CONT, lex_double_quoted_utf8_cont },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_double_quoted_utf8_cont(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	static const struct state_transition_row transitions[] = {
		{ CHAR_ASCII_PRINTABLE, lex_double_quoted_string },
		{ CHAR_WORD_SEPARATOR, lex_double_quoted_string },
		{ CHAR_STATEMENT_SEPARATOR, lex_double_quoted_string },
		{ CHAR_QUOTE, lex_double_quoted_string },
		{ CHAR_UTF8_START, lex_double_quoted_utf8_start },
		{ CHAR_UTF8_CONT, lex_double_quoted_utf8_cont },
		{ CHAR_DOUBLE_QUOTE, lex_end },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_double_quoted_string(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	token.token = SCALLOP_TOKEN_WORD;
	static const struct state_transition_row transitions[] = {
		{ CHAR_ASCII_PRINTABLE, lex_double_quoted_string },
		{ CHAR_WORD_SEPARATOR, lex_double_quoted_string },
		{ CHAR_STATEMENT_SEPARATOR, lex_double_quoted_string },
		{ CHAR_QUOTE, lex_double_quoted_string },
		{ CHAR_UTF8_START, lex_double_quoted_utf8_start },
		{ CHAR_DOUBLE_QUOTE, lex_end_double_quoted_string },
		{ CHAR_NULL, lex_end },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_end_double_quoted_string(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	// identical to quoted_string
	return lex_end_quoted_string(store, token);
}

struct scallop_parse_token lex_word_separator(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	token.token = SCALLOP_TOKEN_WORD_SEPARATOR;
	static const struct state_transition_row transitions[] = {
		{ CHAR_WORD_SEPARATOR, lex_word_separator },
		{ CHAR_ASCII_PRINTABLE, lex_end },
		{ CHAR_QUOTE, lex_end },
		{ CHAR_DOUBLE_QUOTE, lex_end },
		{ CHAR_UTF8_START, lex_end },
		{ CHAR_NULL, lex_end },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_word(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	token.token = SCALLOP_TOKEN_WORD;
	static const struct state_transition_row transitions[] = {
		{ CHAR_ASCII_PRINTABLE, lex_word },
		{ CHAR_UTF8_START, lex_utf8_start },
		{ CHAR_QUOTE, lex_quoted_string },
		{ CHAR_DOUBLE_QUOTE, lex_double_quoted_string },
		{ CHAR_WORD_SEPARATOR, lex_end },
		{ CHAR_STATEMENT_SEPARATOR, lex_end },
		{ CHAR_NULL, lex_end},
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token lex_statement_separator(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	token.token = SCALLOP_TOKEN_STATEMENT_SEPARATOR;
	static const struct state_transition_row transitions[] = {
		{ CHAR_STATEMENT_SEPARATOR, lex_end },
		{ CHAR_ASCII_PRINTABLE, lex_end },
		{ CHAR_UTF8_START, lex_end },
		{ CHAR_QUOTE, lex_end },
		{ CHAR_DOUBLE_QUOTE, lex_end },
		{ CHAR_WORD_SEPARATOR, lex_end },
		{ CHAR_NULL, lex_end },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token token_rewind(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	--token.end_offset;
	char c = get_char(store, token.end_offset);
	char is_newline = c == '\n';
	char is_utf8_cont = c & UTF8_FIRST_BIT;
	token.row -= is_newline;
	token.col = is_newline ? token.col :
		is_utf8_cont ? token.col : token.col - 1;

	return token;
}

struct scallop_parse_token lex_begin(
	csalt_store *store,
	struct scallop_parse_token token
)
{
	// dirty hack to get the right char
	token = token_rewind(store, token);
	const struct next_char current_char = next_char(store, token);
	const enum CHAR_TYPE input = current_char.type;
	token = current_char.token;
	static const struct state_transition_row transitions[] = {
		{ CHAR_ASCII_PRINTABLE, lex_word },
		{ CHAR_UTF8_START, lex_word },
		{ CHAR_QUOTE, lex_quoted_string },
		{ CHAR_DOUBLE_QUOTE, lex_double_quoted_string },
		{ CHAR_WORD_SEPARATOR, lex_word_separator },
		{ CHAR_STATEMENT_SEPARATOR, lex_statement_separator },
		{ CHAR_NULL, lex_eof },
	};

	return transition_state(transitions, input)(store, token);
}

struct scallop_parse_token scallop_lex(
	csalt_store *source,
	struct scallop_parse_token token
)
{
	token.start_offset = token.end_offset;
	return lex_begin(source, token);
}

