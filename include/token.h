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
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_EQUAL,

	TOKEN_INT_LITERAL,

	TOKEN_ID,

	TOKEN_FN_KEYWORD,
	TOKEN_AS_KEYWORD,
	TOKEN_VAR_KEYWORD,
	TOKEN_RETURN_KEYWORD,
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
char *token_type(token_t token);
char *token_lexical(token_t token);

#endif /* TOKEN_H */

