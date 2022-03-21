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
 * Represents a single node in the AST tree.
 * The start_offset and end_offset are offsets
 * into the original string, allowing fetching
 * of the original value from the source.
 *
 * Do not pass read-once stores - such as stdin
 * or network-sockets - to scallop_lex(), as
 * the AST does not store raw data values. If
 * you want to be able to parse read-once stores,
 * you should use csalt_store_fallback with a
 * store that can persist the original source,
 * such as an initialized csalt_resource_vector.
 *
 * You are expected to use start_offset and
 * end_offset to read out the raw values from the
 * original store.
 */
struct scallop_ast_node {
	int32_t token;
	int32_t start_offset;
	int32_t end_offset;
};

/**
 * This function takes a human-readable Scallop
 * string, stored in source, and parses it into
 * an AST. The AST is stored as a simple, flat
 * list; convenience functions are provided for
 * traversing the tree.
 *
 * Do not pass read-once stores to scallop_lex().
 * The returned AST includes indices back into
 * the original string to allow value parsing.
 *
 * \sa scallop_parse_parameter
 */
int scallop_lex(
	csalt_store *source,
	csalt_store_block_fn *callback,
	void *param
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SCALLOP_LEXER_H
