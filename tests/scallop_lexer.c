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

#include "lexer.h"

#include <csalt/stores.h>
#include <stdlib.h>
#include <stdio.h>

#include "scallop_util.h"

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

int run_first_test(csalt_store *result, void *string_ptr)
{
	const struct scallop_ast_node expected_node_list[] = {
		{ SCALLOP_TOKEN_WORD, 0, 5 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 5, 6 },
		{ SCALLOP_TOKEN_WORD, 6, 18 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 18, 19 },
		{ SCALLOP_TOKEN_WORD, 19, 29 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 29, 30 },
		{ SCALLOP_TOKEN_OPENING_SQUARE_BRACKET, 30, 31 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 31, 32 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 32, 33 },
		{ SCALLOP_TOKEN_WORD, 33, 39 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 39, 40 },
		{ SCALLOP_TOKEN_WORD, 40, 58 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 58, 59 },
		{ SCALLOP_TOKEN_OPENING_SQUARE_BRACKET, 59, 60 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 60, 61 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 61, 63 },
		{ SCALLOP_TOKEN_WORD, 63, 71 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 71, 72 },
		{ SCALLOP_TOKEN_WORD, 72, 82 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 82, 83 },
		{ SCALLOP_TOKEN_OPENING_CURLY_BRACKET, 83, 84 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 84, 85 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 85, 88 },
		{ SCALLOP_TOKEN_WORD, 88, 98 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 98, 99 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 99, 100 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 100, 103 },
		{ SCALLOP_TOKEN_WORD, 103, 106 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 106, 107 },
		{ SCALLOP_TOKEN_WORD, 107, 116 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 116, 117 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 117, 118 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 118, 120 },
		{ SCALLOP_TOKEN_CLOSING_CURLY_BRACKET, 120, 121 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 121, 122 },
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 122, 123 },
		{ SCALLOP_TOKEN_CLOSING_SQUARE_BRACKET, 123, 124 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 124, 125 },
		{ SCALLOP_TOKEN_CLOSING_SQUARE_BRACKET, 125, 126 },
		{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 126, 127 },
	};

	struct csalt_memory *string = string_ptr;
	struct scallop_ast_node node = { 0 };

	int index = 0;
	for (
		const struct scallop_ast_node
			*current_expected = expected_node_list,
			*end_expected = arrend(expected_node_list);
		current_expected < end_expected;
		++current_expected, ++index
	) {
		if (!store_get_element(result, &node, index, sizeof(node))) {
			print_error("Failed to get node at index %d", index);
			return EXIT_FAILURE;
		}

		if (node.token != current_expected->token) {
			print_error(
				"Unexpected token type at character %d, expected: %s | actual: %s",
				node.start_offset,
				token_types[current_expected->token],
				token_types[node.token]
			);
			return EXIT_FAILURE;
		}

		int position_correct =
			node.start_offset == current_expected->start_offset &&
			node.end_offset == current_expected->end_offset;

		if (!position_correct) {
			print_error(
				"Unexpected offsets, expected: %d->%d | actual: %d->%d",
				current_expected->start_offset,
				current_expected->end_offset,
				node.start_offset,
				node.end_offset
			);
			return EXIT_FAILURE;
		}
	}


	return EXIT_SUCCESS;
}

int main()
{
	{
		const char sample_script[] =
			"https --ip=0.0.0.0 --port=443 [\n"
			"	domain \"🎉.example.com\" [\n"
			"		location '/.*\\.php' {\n"
			"			log_access;\n"
			"			php $location;\n"
			"		}\n"
			"	]\n"
			"];";

		struct csalt_cmemory csalt_string = csalt_cmemory_array(sample_script);

		int error = scallop_lex((csalt_store *)&csalt_string, run_first_test, &csalt_string);
		if (error) {
			print_error("Error parsing valid script");
			return error;
		}
	}
}
