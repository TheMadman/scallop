#include "test_macros.h"

#include <csalt/stores.h>

int main()
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

