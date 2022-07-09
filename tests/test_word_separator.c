#include "test_macros.h"

#include <csalt/stores.h>

int main()
{
	static const char script[] = " \t";
	
	expect(script,
		{ SCALLOP_TOKEN_WORD_SEPARATOR, 0, 2 },
		{ SCALLOP_TOKEN_EOF, 2, 3 }
	);
}


