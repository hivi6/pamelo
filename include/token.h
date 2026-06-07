#ifndef TOKEN_H
#define TOKEN_H

#include "pos.h"
#include "util.h"

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
	TOKEN_COMMA,
	TOKEN_STAR,
	TOKEN_FSLASH,
	TOKEN_MOD,

	TOKEN_EQUAL_EQUAL,
	TOKEN_BANG_EQUAL,

	TOKEN_INT_LITERAL,

	TOKEN_ID,

	TOKEN_FN_KEYWORD,
	TOKEN_AS_KEYWORD,
	TOKEN_VAR_KEYWORD,
	TOKEN_RETURN_KEYWORD,
	TOKEN_EXTERN_KEYWORD,
	TOKEN_IF_KEYWORD,
	TOKEN_ELSE_KEYWORD,
	TOKEN_WHILE_KEYWORD,
};

typedef struct token_t token_t;
struct token_t {
	int kind;
	const char *filepath;
	const char *source;
	pos_t start;
	pos_t end;
};

vec_t generate_tokens(const char *filepath, const char *source);
char *token_type(token_t token);
char *token_lexical(token_t token);
void print_tokens(vec_t tokens);

#endif /* TOKEN_H */

