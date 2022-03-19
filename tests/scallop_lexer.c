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

const char sample_script[] =
	"{\n"
	"	{\n"
	"		input_from_mouse;\n"
	"		input_from_keyboard;\n"
	"		input_from_network;\n"
	"	} || {\n"
	"		simulate;\n"
	"	} || {\n"
	"		render_to_screen;\n"
	"		render_to_audio;\n"
	"		render_to_network;\n"
	"	}\n"
	"}";

const struct scallop_ast_node expected_node_list[] = {
	{ SCALLOP_TOKEN_OPENING_BRACE, 0, 1 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 1, 2 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 2, 3 },
	{ SCALLOP_TOKEN_OPENING_BRACE, 3, 4 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 4, 5 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 5, 7 },
	{ SCALLOP_TOKEN_WORD, 7,  23 }, // 7 + sizeof("input_from_mouse") - 1
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 23, 24 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 24, 25 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 25, 27 },
	{ SCALLOP_TOKEN_WORD, 27, 46 }, // 27 + sizeof("input_from_keyboard") - 1
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 46, 47 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 47, 48 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 48, 50 },
	{ SCALLOP_TOKEN_WORD, 50, 68 }, // 50 + sizeof("input_from_network") - 1
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 68, 69 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 69, 70 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 70, 71 },
	{ SCALLOP_TOKEN_CLOSING_BRACE, 71, 72 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 72, 73 },
	{ SCALLOP_TOKEN_BINARY_PIPE, 73, 75 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 75, 76 },
	{ SCALLOP_TOKEN_OPENING_BRACE, 76, 77 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 77, 78 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 78, 80 },
	{ SCALLOP_TOKEN_WORD, 80, 88 }, // 80 + sizeof("simulate") - 1
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 88, 89 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 89, 90 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 90, 91 },
	{ SCALLOP_TOKEN_CLOSING_BRACE, 91, 92 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 92, 93 },
	{ SCALLOP_TOKEN_BINARY_PIPE, 93, 95 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 95, 96 },
	{ SCALLOP_TOKEN_OPENING_BRACE, 96, 97 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 97, 98 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 98, 100 },
	{ SCALLOP_TOKEN_WORD, 100, 116 }, // 100 + sizeof("render_to_screen") - 1
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 116, 117 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 117, 118 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 118, 120 },
	{ SCALLOP_TOKEN_WORD, 120, 135 }, // 120 + sizeof("render_to_audio") -1
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 135, 136 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 136, 137 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 137, 139 },
	{ SCALLOP_TOKEN_WORD, 139, 156 }, // 139 + sizeof("render_to_network") -1
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 156, 157 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 157, 158 },
	{ SCALLOP_TOKEN_WORD_SEPARATOR, 158, 159 },
	{ SCALLOP_TOKEN_CLOSING_BRACE, 159, 160 },
	{ SCALLOP_TOKEN_STATEMENT_SEPARATOR, 160, 161 },
	{ SCALLOP_TOKEN_CLOSING_BRACE, 161, 162 },
};

const char *token_types[] = {
	"SCALLOP_TOKEN_WORD",
	"SCALLOP_TOKEN_WORD_SEPARATOR",
	"SCALLOP_TOKEN_STATEMENT_SEPARATOR",
	"SCALLOP_TOKEN_OPENING_BRACE",
	"SCALLOP_TOKEN_CLOSING_BRACE",
	"SCALLOP_TOKEN_ASSIGNMENT_OPERATOR",
};

int run_first_test(csalt_store *result, void *string_ptr)
{
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
		struct csalt_cmemory csalt_string = csalt_cmemory_array(sample_script);

		int error = scallop_lex((csalt_store *)&csalt_string, run_first_test, &csalt_string);
		if (error) {
			print_error("Error parsing valid script");
			return error;
		}
	}
}
