#include "test_macros.h"

#include <csalt/stores.h>

int main()
{
	static const char script[] = "'\\\'a\\a \\z;\\b'";
	expect(script,
		{ SCALLOP_TOKEN_WORD, 0, 12 },
		{ SCALLOP_TOKEN_EOF, 12, 13 }
	);
}

