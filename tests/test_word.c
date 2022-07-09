#include "test_macros.h"

#include <csalt/stores.h>

int main()
{
	static const char script[] = "foo";

	expect(script,
		{ SCALLOP_TOKEN_WORD, 0, 3 },
		{ SCALLOP_TOKEN_EOF, 3, 4 }
	);
}

