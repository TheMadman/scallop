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

struct next_state {
	void_fn *state;
	struct scallop_parse_token token;
};

typedef struct next_state lex_fn(struct next_char);

struct next_state lex_error(struct next_char next_char)
{
	assert(!"`lex_fn *lex_error` should never be called!");
	return (struct next_state) {
		(void_fn *)lex_error,
		next_char.token,
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
	lex_fn *state;
	enum CHAR_TYPE input;
	lex_fn *new_state;
};

static void_fn *transition_state_bounds(
	struct state_transition_row *rows_begin,
	struct state_transition_row *rows_end,
	lex_fn *current_state,
	enum CHAR_TYPE input
)
{
	for (
		struct state_transition_row *current = rows_begin;
		current < rows_end;
		current++
	) {
		if (
			current_state == current->state &&
			input == current->input
		) {
			return (void_fn *)current->new_state;
		}
	}
	return (void_fn *)lex_error;
}

#define transition_state(array, current_state, input) \
	transition_state_bounds((array), arrend(array), current_state, input)

struct next_state lex_end(
	struct next_char
);
struct next_state lex_utf8_start(
	struct next_char
);
struct next_state lex_utf8_cont(
	struct next_char
);
struct next_state lex_quoted_utf8_start(
	struct next_char
);
struct next_state lex_quoted_utf8_cont(
	struct next_char
);
struct next_state lex_word_separator(
	struct next_char
);
struct next_state lex_statement_separator(
	struct next_char
);
struct next_state lex_end_quoted_string(
	struct next_char next_char
);
struct next_state lex_quoted_string(
	struct next_char next_char
);
struct next_state lex_double_quoted_utf8_start(
	struct next_char next_char
);
struct next_state lex_double_quoted_utf8_cont(
	struct next_char next_char
);
struct next_state lex_double_quoted_string(
	struct next_char next_char
);
struct next_state lex_end_double_quoted_string(
	struct next_char next_char
);
struct next_state lex_word(
	struct next_char
);
struct next_state lex_begin(
	struct next_char next_char
);

struct next_state lex_end(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	token.token = SCALLOP_TOKEN_EOF;
	return (struct next_state) {
		(void_fn *)lex_end,
		token,
	};
}

struct next_state lex_utf8_start(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_utf8_start, CHAR_UTF8_CONT, lex_utf8_cont },
	};

	return (struct next_state) {
		transition_state(transitions, lex_utf8_start, input),
		token,
	};
}

struct next_state lex_utf8_cont(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_utf8_cont, CHAR_UTF8_START, lex_utf8_start },
		{ lex_utf8_cont, CHAR_UTF8_CONT, lex_utf8_cont },
		{ lex_utf8_cont, CHAR_ASCII_PRINTABLE, lex_word },
		{ lex_utf8_cont, CHAR_WORD_SEPARATOR, lex_word_separator },
		{ lex_utf8_cont, CHAR_STATEMENT_SEPARATOR, lex_statement_separator },
		{ lex_utf8_cont, CHAR_NULL, lex_end },
	};

	return (struct next_state) {
		transition_state(transitions, lex_utf8_cont, input),
		token,
	};
}

struct next_state lex_quoted_utf8_start(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_quoted_utf8_start, CHAR_UTF8_CONT, lex_quoted_utf8_cont },
	};

	return (struct next_state) {
		transition_state(transitions, lex_quoted_utf8_start, input),
		token,
	};
}

struct next_state lex_quoted_utf8_cont(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_quoted_utf8_cont, CHAR_UTF8_CONT, lex_quoted_utf8_cont },
		{ lex_quoted_utf8_cont, CHAR_ASCII_PRINTABLE, lex_quoted_string },
		{ lex_quoted_utf8_cont, CHAR_WORD_SEPARATOR, lex_quoted_string },
		{ lex_quoted_utf8_cont, CHAR_STATEMENT_SEPARATOR, lex_quoted_string },
		{ lex_quoted_utf8_cont, CHAR_DOUBLE_QUOTE, lex_quoted_string },
		{ lex_quoted_utf8_cont, CHAR_QUOTE, lex_end_quoted_string },
		{ lex_quoted_utf8_cont, CHAR_WORD_SEPARATOR, lex_quoted_string },
	};

	return (struct next_state) {
		transition_state(transitions, lex_quoted_utf8_cont, input),
		token,
	};
}

struct next_state lex_end_quoted_string(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_end_quoted_string, CHAR_ASCII_PRINTABLE, lex_word },
		{ lex_end_quoted_string, CHAR_UTF8_START, lex_utf8_start },
		{ lex_end_quoted_string, CHAR_QUOTE, lex_quoted_string },
		{ lex_end_quoted_string, CHAR_WORD_SEPARATOR, lex_word_separator },
		{ lex_end_quoted_string, CHAR_STATEMENT_SEPARATOR, lex_statement_separator },
		{ lex_end_quoted_string, CHAR_NULL, lex_end },

	};

	return (struct next_state) {
		transition_state(transitions, lex_end_quoted_string, input),
		token,
	};
}

struct next_state lex_quoted_string(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	token.token = SCALLOP_TOKEN_WORD;
	struct state_transition_row transitions[] = {
		{ lex_quoted_string, CHAR_ASCII_PRINTABLE, lex_quoted_string },
		{ lex_quoted_string, CHAR_UTF8_START, lex_quoted_utf8_start },
		{ lex_quoted_string, CHAR_WORD_SEPARATOR, lex_quoted_string },
		{ lex_quoted_string, CHAR_STATEMENT_SEPARATOR, lex_quoted_string },
		// { lex_quoted_string, CHAR_BACKSLASH, lex_escape },
		{ lex_quoted_string, CHAR_DOUBLE_QUOTE, lex_quoted_string },
		{ lex_quoted_string, CHAR_QUOTE, lex_end_quoted_string },
		{ lex_quoted_string, CHAR_NULL, lex_end },
	};

	return (struct next_state) {
		transition_state(transitions, lex_quoted_string, input),
		token,
	};
}

struct next_state lex_double_quoted_utf8_start(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_double_quoted_utf8_start, CHAR_UTF8_CONT, lex_double_quoted_utf8_cont },
	};

	return (struct next_state) {
		transition_state(transitions, lex_double_quoted_utf8_start, input),
		token,
	};
}

struct next_state lex_double_quoted_utf8_cont(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_double_quoted_utf8_cont, CHAR_ASCII_PRINTABLE, lex_double_quoted_string },
		{ lex_double_quoted_utf8_cont, CHAR_WORD_SEPARATOR, lex_double_quoted_string },
		{ lex_double_quoted_utf8_cont, CHAR_STATEMENT_SEPARATOR, lex_double_quoted_string },
		{ lex_double_quoted_utf8_cont, CHAR_QUOTE, lex_double_quoted_string },
		{ lex_double_quoted_utf8_cont, CHAR_UTF8_START, lex_double_quoted_utf8_start },
		{ lex_double_quoted_utf8_cont, CHAR_UTF8_CONT, lex_double_quoted_utf8_cont },
		{ lex_double_quoted_utf8_cont, CHAR_DOUBLE_QUOTE, lex_begin },
	};

	return (struct next_state) {
		transition_state(transitions, lex_double_quoted_utf8_cont, input),
		token,
	};
}

struct next_state lex_double_quoted_string(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	token.token = SCALLOP_TOKEN_WORD;
	struct state_transition_row transitions[] = {
		{ lex_double_quoted_string, CHAR_ASCII_PRINTABLE, lex_double_quoted_string },
		{ lex_double_quoted_string, CHAR_WORD_SEPARATOR, lex_double_quoted_string },
		{ lex_double_quoted_string, CHAR_STATEMENT_SEPARATOR, lex_double_quoted_string },
		{ lex_double_quoted_string, CHAR_QUOTE, lex_double_quoted_string },
		{ lex_double_quoted_string, CHAR_UTF8_START, lex_double_quoted_utf8_start },
		{ lex_double_quoted_string, CHAR_DOUBLE_QUOTE, lex_end_double_quoted_string },
		{ lex_double_quoted_string, CHAR_NULL, lex_end },
	};

	return (struct next_state) {
		transition_state(transitions, lex_double_quoted_string, input),
		token,
	};
}

struct next_state lex_end_double_quoted_string(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_end_double_quoted_string, CHAR_ASCII_PRINTABLE, lex_word },
		{ lex_end_double_quoted_string, CHAR_UTF8_START, lex_utf8_start },
		{ lex_end_double_quoted_string, CHAR_WORD_SEPARATOR, lex_word_separator },
		{ lex_end_double_quoted_string, CHAR_STATEMENT_SEPARATOR, lex_statement_separator },
		{ lex_end_double_quoted_string, CHAR_QUOTE, lex_quoted_string },
		{ lex_end_double_quoted_string, CHAR_DOUBLE_QUOTE, lex_double_quoted_string, },
		{ lex_end_double_quoted_string, CHAR_NULL, lex_end },
	};

	return (struct next_state) {
		transition_state(transitions, lex_end_double_quoted_string, input),
		token,
	};
}

struct next_state lex_word_separator(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	token.token = SCALLOP_TOKEN_WORD_SEPARATOR;
	struct state_transition_row transitions[] = {
		{ lex_word_separator, CHAR_WORD_SEPARATOR, lex_word_separator },
		{ lex_word_separator, CHAR_ASCII_PRINTABLE, lex_word },
		{ lex_word_separator, CHAR_QUOTE, lex_quoted_string },
		{ lex_word_separator, CHAR_DOUBLE_QUOTE, lex_double_quoted_string },
		{ lex_word_separator, CHAR_UTF8_START, lex_utf8_start },
		{ lex_word_separator, CHAR_NULL, lex_end },
	};

	return (struct next_state) {
		transition_state(transitions, lex_word_separator, input),
		token,
	};
}

struct next_state lex_word(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	token.token = SCALLOP_TOKEN_WORD;
	struct state_transition_row transitions[] = {
		{ lex_word, CHAR_ASCII_PRINTABLE, lex_word },
		{ lex_word, CHAR_UTF8_START, lex_utf8_start },
		{ lex_word, CHAR_QUOTE, lex_quoted_string },
		{ lex_word, CHAR_DOUBLE_QUOTE, lex_double_quoted_string },
		{ lex_word, CHAR_WORD_SEPARATOR, lex_word_separator },
		{ lex_word, CHAR_STATEMENT_SEPARATOR, lex_statement_separator },
		{ lex_word, CHAR_NULL, lex_end},
	};

	return (struct next_state) {
		transition_state(transitions, lex_word, input),
		token,
	};
}

struct next_state lex_statement_separator(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	token.token = SCALLOP_TOKEN_STATEMENT_SEPARATOR;
	struct state_transition_row transitions[] = {
		{ lex_statement_separator, CHAR_STATEMENT_SEPARATOR, lex_statement_separator },
		{ lex_statement_separator, CHAR_ASCII_PRINTABLE, lex_word },
		{ lex_statement_separator, CHAR_UTF8_START, lex_utf8_start },
		{ lex_statement_separator, CHAR_QUOTE, lex_quoted_string },
		{ lex_statement_separator, CHAR_DOUBLE_QUOTE, lex_double_quoted_string },
		{ lex_statement_separator, CHAR_WORD_SEPARATOR, lex_word_separator },
		{ lex_statement_separator, CHAR_NULL, lex_end },
	};

	return (struct next_state) {
		transition_state(transitions, lex_statement_separator, input),
		token,
	};
}

struct next_state lex_begin(
	struct next_char next_char
)
{
	enum CHAR_TYPE input = next_char.type;
	struct scallop_parse_token token = next_char.token;
	struct state_transition_row transitions[] = {
		{ lex_begin, CHAR_ASCII_PRINTABLE, lex_word },
		{ lex_begin, CHAR_UTF8_START, lex_word },
		{ lex_begin, CHAR_QUOTE, lex_quoted_string },
		{ lex_begin, CHAR_DOUBLE_QUOTE, lex_double_quoted_string },
		{ lex_begin, CHAR_WORD_SEPARATOR, lex_word_separator },
		{ lex_begin, CHAR_STATEMENT_SEPARATOR, lex_statement_separator },
		{ lex_begin, CHAR_NULL, lex_end },
	};

	lex_fn *start = (lex_fn *)transition_state(
		transitions,
		lex_begin,
		input
	);

	return start(next_char);
}

int scallop_lex(
	csalt_store *source,
	scallop_parse_fn *parse_function,
	void *param
)
{
	// Initializing the first token is just a pain, and
	// I can't figure out a good way to do it as part of the
	// "core logic" of this function
	struct scallop_parse_token current = { .end_offset = -1, .row = 1 };
	struct next_char next = next_char(source, current);
	struct next_state look_ahead = lex_begin(next);
	current = look_ahead.token;

	for (lex_fn *lexer = (lex_fn *)look_ahead.state; parse_function;) {
		if (current.token == SCALLOP_TOKEN_EOF)
			return 0;

		next = next_char(source, current);
		look_ahead = lexer(next);

		if (look_ahead.token.token != current.token) {
			parse_function = (scallop_parse_fn *)parse_function(
				source,
				current,
				param
			);
			look_ahead.token.start_offset = current.end_offset;
		}

		current = look_ahead.token;
		lexer = (lex_fn *)look_ahead.state;
	}

	if (!parse_function)
		return -1;
	return 0;
}

