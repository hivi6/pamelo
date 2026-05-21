#ifndef TOKEN_H
#define TOKEN_H

#include "pos.h"

enum {
	TOKEN_EOF,

	TOKEN_LBRACE,
	TOKEN_LPAREN,
	TOKEN_RBRACE,
	TOKEN_RPAREN,
	TOKEN_SEMICOLON,

	TOKEN_INT_LITERAL,

	TOKEN_FN_KEYWORD,
};

typedef struct token_t token_t;
struct token_t {
	int kind;
	const char *filepath;
	const char *source;
	pos_t start;
	pos_t end;

	// used for getting the next token in the list
	token_t *next;
};

token_t *generate_tokens(const char *filepath, const char *source);

#endif /* TOKEN_H */

