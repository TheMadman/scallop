#include <stdio.h>
#include <assert.h>

#include "scallop/util.h"
#include "scallop/lexer.h"

#define print_error(format, ...) fprintf(stderr, "%s:%d: " format "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)

#define assert_tokens_equal(expected, actual) \
{ \
	assert((expected).token == (actual).token); \
	assert((expected).start_offset == (actual).start_offset); \
	assert((expected).end_offset == (actual).end_offset); \
}

static const char *token_types[] = {
	"SCALLOP_TOKEN_EOF",
	"SCALLOP_TOKEN_WORD",
	"SCALLOP_TOKEN_WORD_SEPARATOR",
	"SCALLOP_TOKEN_STATEMENT_SEPARATOR",
	"SCALLOP_TOKEN_OPEN_CURLY_BRACKET",
	"SCALLOP_TOKEN_CLOSE_CURLY_BRACKET",
	"SCALLOP_TOKEN_OPEN_SQUARE_BRACKET",
	"SCALLOP_TOKEN_CLOSE_SQUARE_BRACKET",
	"SCALLOP_TOKEN_ASSIGNMENT_OPERATOR",
	"SCALLOP_TOKEN_PIPE",
	"SCALLOP_TOKEN_BINARY_PIPE",
};

// This upsets the syntax highlighter of (n)vim.
// Not gonna lie, it kinda upsets me too.
#define expect(script, ...) \
{ \
	struct csalt_cmemory csalt_script = csalt_cmemory_array(script); \
	csalt_store * const store = (csalt_store *)&csalt_script; \
	static const struct scallop_parse_token expects[] = { \
		__VA_ARGS__ \
	}; \
	const struct scallop_parse_token *expected = expects; \
	struct scallop_parse_token actual = { 0 }; \
	for ( \
		actual = scallop_lex(store, actual); \
		actual.token != SCALLOP_TOKEN_EOF; \
		actual = scallop_lex(store, actual), ++expected \
	) { \
		print_error( \
			"expected: %s %ld -> %ld", \
			token_types[expected->token], \
			expected->start_offset, \
			expected->end_offset \
		); \
		print_error( \
			"actual: %s %ld -> %ld", \
			token_types[actual.token], \
			actual.start_offset, \
			actual.end_offset \
		); \
		assert_tokens_equal(*expected, actual); \
	} \
}


