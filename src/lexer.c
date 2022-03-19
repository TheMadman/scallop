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
#include "lexer.h"

#include <stdlib.h>
#include <csalt/resources.h>

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
	CHAR_OPEN_BRACE,
	CHAR_CLOSE_BRACE,
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
			return CHAR_OPEN_BRACE;
		case '}':
			return CHAR_CLOSE_BRACE;
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

static ssize_t consume_utf8_chars(csalt_store *string, ssize_t begin);
static ssize_t consume_word(csalt_store *string, ssize_t begin);
static ssize_t consume_double_quote_string(csalt_store *string, ssize_t begin);
static ssize_t consume_single_quote_string(csalt_store *string, ssize_t begin);
static ssize_t consume_word_separators(csalt_store *string, ssize_t begin);

static ssize_t consume_utf8_chars(csalt_store *string, ssize_t begin)
{
	for (
		char
			current = get_char(string, ++begin),
			char_count = 1;
		/* noop */;
		current = get_char(string, ++begin),
		++char_count
	) {
		switch (char_type(current)) {
			case CHAR_UTF8_START:
				if (char_count < 2)
					return -1;
				break;
			case CHAR_UTF8_CONT:
				break;
			case CHAR_NULL:
			case CHAR_UNKNOWN:
				return -1;
			default:
				return begin;
		}
	}
}

static ssize_t consume_word(csalt_store *string, ssize_t begin)
{
	for (
		char current = get_char(string, begin);
		begin >= 0;
		current = get_char(string, ++begin)
	) {
		switch (char_type(current)) {
			case CHAR_UTF8_START:
				// I don't like the -1 here, but it's necessary and
				// I can't think of a better way to phrase it without
				// breaking a convention
				begin = consume_utf8_chars(string, begin) - 1;
				break;
			case CHAR_UTF8_CONT:
				return -1;
			case CHAR_QUOTE:
				// string-literals against words with no separator are
				// concatenated
				// e.g. foo'bar'baz == foobarbaz
				begin = consume_single_quote_string(string, begin) - 1;
				break;
			case CHAR_DOUBLE_QUOTE:
				// string-literals against words with no separator are
				// concatenated
				// e.g. foo"bar"baz == foobarbaz
				begin = consume_double_quote_string(string, begin) - 1;
				break;
			case CHAR_BACKSLASH:
				++begin;
			case CHAR_STATEMENT_SEPARATOR:
			case CHAR_WORD_SEPARATOR:
			case CHAR_NULL:
				return begin;
			default:
				break;
		}
	}

	// begin contains value < 0, error value
	return -1;
}

static ssize_t consume_string_literal(csalt_store *string, ssize_t begin, char terminator)
{
	enum CHAR_TYPE terminator_type = char_type(terminator);
	for (
		// ++begin to skip the first character (opening quote)
		char current = get_char(string, ++begin);
		/* noop */;
		current = get_char(string, ++begin)
	) {
		enum CHAR_TYPE current_char_type = char_type(current);
		if (current_char_type == terminator_type)
			return begin;

		switch (current_char_type) {
			case CHAR_BACKSLASH:
				// skip escaped character
				++begin;
				break;
			case CHAR_NULL:
			case CHAR_UNKNOWN:
			case CHAR_UTF8_CONT:
				return -1;
			default:
				// continue looping
				break;
		}
	}
}

static ssize_t consume_double_quote_string(csalt_store *string, ssize_t begin)
{
	return consume_string_literal(string, begin, '"');
}

static ssize_t consume_single_quote_string(csalt_store *string, ssize_t begin)
{
	return consume_string_literal(string, begin, '\'');
}

static ssize_t consume_word_separators(csalt_store *string, ssize_t begin)
{
	for (
		char current = get_char(string, begin);
		/* noop */;
		current = get_char(string, ++begin)
	) {
		switch (char_type(current)) {
			case CHAR_WORD_SEPARATOR:
				break;
			default:
				return begin;
		}
	}
}

static struct scallop_ast_node disambiguate_pipe(csalt_store *string, ssize_t begin)
{
	char current_char = get_char(string, ++begin);
	switch (char_type(current_char)) {
		case CHAR_PIPE:
			return (struct scallop_ast_node) {
				SCALLOP_TOKEN_BINARY_PIPE,
				begin - 1,
				begin + 1,
			};
		default:
			return (struct scallop_ast_node) {
				SCALLOP_TOKEN_PIPE,
				begin - 1,
				begin,
			};
	}
}

static struct scallop_ast_node chars_to_token(csalt_store *string, ssize_t begin)
{
	struct scallop_ast_node result = { 0 };
	char current_char = get_char(string, begin);
	switch (char_type(current_char)) {
		case CHAR_OPEN_BRACE:
			result = (struct scallop_ast_node) {
				SCALLOP_TOKEN_OPENING_BRACE,
				begin,
				begin + 1
			};
			break;
		case CHAR_CLOSE_BRACE:
			result = (struct scallop_ast_node) {
				SCALLOP_TOKEN_CLOSING_BRACE,
				begin,
				begin + 1
			};
			break;
		case CHAR_QUOTE:
			result = (struct scallop_ast_node) {
				SCALLOP_TOKEN_WORD,
				begin,
				consume_word(string, begin)
			};
			break;
		case CHAR_DOUBLE_QUOTE:
			result = (struct scallop_ast_node) {
				SCALLOP_TOKEN_WORD,
				begin,
				consume_word(string, begin)
			};
			break;
		case CHAR_ASCII_PRINTABLE:
		case CHAR_UTF8_START:
		case CHAR_BACKSLASH:
			result = (struct scallop_ast_node) {
				SCALLOP_TOKEN_WORD,
				begin,
				consume_word(string, begin)
			};
			break;
		case CHAR_WORD_SEPARATOR:
			result = (struct scallop_ast_node) {
				SCALLOP_TOKEN_WORD_SEPARATOR,
				begin,
				consume_word_separators(string, begin)
			};
			break;
		case CHAR_STATEMENT_SEPARATOR:
			result = (struct scallop_ast_node) {
				SCALLOP_TOKEN_STATEMENT_SEPARATOR,
				begin,
				begin + 1
			};
			break;
		case CHAR_PIPE:
			result = disambiguate_pipe(string, begin);
			break;
		case CHAR_UNKNOWN:
			result.end_offset = -1;
			break;
		default:
			break;
	}
	return result;
}

struct vector_write_params {
	const struct scallop_ast_node *node;
	
	ssize_t write_result;
};

static int receive_split_vector_for_write(csalt_store *store, void *param)
{
	struct vector_write_params *params = param;
	const struct scallop_ast_node *node = params->node;
	params->write_result = csalt_store_write(store, node, sizeof(*node));
	return params->write_result == sizeof(*node);
}

static ssize_t write_node_into_vector(
	csalt_store *store,
	const struct scallop_ast_node node,
	ssize_t index
)
{
	ssize_t begin_byte = index * sizeof(node);
	ssize_t end_byte = (index + 1) * sizeof(node);
	struct vector_write_params params = {
		&node,
		-1,
	};
	csalt_store_split(
		store,
		begin_byte,
		end_byte,
		receive_split_vector_for_write,
		&params
	);
	return params.write_result;
}

struct scallop_parse_param {
	csalt_store *store;
	csalt_store_block_fn *block;
	void *param;
};

static int lex_into_vector(csalt_store *store, void *param)
{
	struct scallop_parse_param *wrapped_param = param;
	csalt_store *source = wrapped_param->store;

	ssize_t source_size = csalt_store_size(source);

	for (ssize_t current = 0, index = 0; current < source_size - 1; ++index) {
		struct scallop_ast_node current_node = chars_to_token(source, current);
		if (current_node.end_offset == -1)
			return -1;
		ssize_t write_result = write_node_into_vector(store, current_node, index);
		if (write_result != sizeof(current_node))
			return -1;

		current = current_node.end_offset;
	}
	return wrapped_param->block(store, wrapped_param->param);
}

int scallop_lex(
	csalt_store *source,
	csalt_store_block_fn *block,
	void *param
)
{
	struct scallop_parse_param wrapped_param = {
		source,
		block,
		param,
	};

	struct csalt_resource_vector
		vector = csalt_resource_vector(0);

	return csalt_resource_use(
		(csalt_resource *)&vector,
		lex_into_vector,
		&wrapped_param
	);
}
