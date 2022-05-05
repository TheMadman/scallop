/*
 * Scallop - a shell for executing tasks concurrently
 * Copyright (C) 2022  Marcus Harrison
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
#ifndef SCALLOP_LEXER_H
#define SCALLOP_LEXER_H

#include <csalt/stores.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum SCALLOP_TOKEN {
	SCALLOP_TOKEN_EOF,
	SCALLOP_TOKEN_WORD,
	SCALLOP_TOKEN_WORD_SEPARATOR,
	SCALLOP_TOKEN_STATEMENT_SEPARATOR,
	SCALLOP_TOKEN_OPENING_CURLY_BRACKET,
	SCALLOP_TOKEN_CLOSING_CURLY_BRACKET,
	SCALLOP_TOKEN_OPENING_SQUARE_BRACKET,
	SCALLOP_TOKEN_CLOSING_SQUARE_BRACKET,
	SCALLOP_TOKEN_ASSIGNMENT_OPERATOR,
	SCALLOP_TOKEN_PIPE,
	SCALLOP_TOKEN_BINARY_PIPE,
};

/**
 * \brief Represents a single token.
 *
 * The start_offset and end_offset are offsets
 * into the original string, allowing fetching
 * of the original value from the source.
 *
 * Do not pass read-once stores - such as stdin
 * or network-sockets - to scallop_lex(), as
 * the token does not store raw data values. If
 * you want to be able to parse read-once stores,
 * you should use csalt_store_fallback with a
 * store that can persist the original source,
 * such as an initialized csalt_resource_vector.
 *
 * You are expected to use start_offset and
 * end_offset to read out the raw values from the
 * original store.
 */
struct scallop_parse_token {
	int32_t token;
	int64_t start_offset;
	int64_t end_offset;
	int64_t row;
	int64_t col;
};

typedef void void_fn(void);

/**
 * \brief This type represents a parse function.
 *
 * it takes a single scallop_parse_token and an arbitrary
 * void pointer and returns a pointer to the next parse
 * function, or a null pointer if there was an error.
 *
 * Note that you should cast your return value to this
 * type to avoid warnings.
 */
typedef void_fn *scallop_parse_fn(
	csalt_store *,
	struct scallop_parse_token,
	void *
);

/**
 * \brief Lexes UTF-8 text in source into tokens, one at
 * 	a time, and passed to parse functions, the first
 * 	of which is passed as first_parse_functions.
 *
 * first_parse_function will take the first token and
 * should return a pointer to the function expecting the next token,
 * or a null pointer if the first token was invalid.
 */
int scallop_lex(
	csalt_store *source,
	scallop_parse_fn *first_parse_function,
	void *param
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SCALLOP_LEXER_H
