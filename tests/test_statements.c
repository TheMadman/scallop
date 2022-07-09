#include "test_macros.h"

#include <csalt/stores.h>

int main()
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

